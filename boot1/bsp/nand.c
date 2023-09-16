#include <string.h>

#include "bsp.h"
#include "config.h"
#include "utils.h"

//#include <assert.h>

#undef assert
#define assert(x) 

// #define page_sz  (2048)

#define CHIP_SEL 0
#define NAND_DMA_Channel 4
#define MASK_AND_REFERENCE_VALUE 0x0100

#define NAND_CMD_READ0 0
#define NAND_CMD_READSTART 0x30

#define NAND_CMD_ERASE1 0x60
#define NAND_CMD_ERASE2 0xd0

#define NAND_CMD_SEQIN 0x80
#define NAND_CMD_PAGEPROG 0x10

#define NAND_CMD_STATUS 0x70

uint32_t page_sz = 2048;

typedef struct GPMI_DMA_Desc {
    // DMA related fields
    uint32_t dma_nxtcmdar;
    uint32_t dma_cmd;
    uint32_t dma_bar;
    // GPMI related fields
    uint32_t gpmi_ctrl0;    // PIO 0
    uint32_t gpmi_compare;  // PIO 1
    uint32_t gpmi_eccctrl;  // PIO 2
    uint32_t gpmi_ecccount; // PIO 3
    uint32_t gpmi_data_ptr; // PIO 4
    uint32_t gpmi_aux_ptr;  // PIO 5
} GPMI_DMA_Desc;

volatile GPMI_DMA_Desc *chains_cmd;
volatile GPMI_DMA_Desc *chains_read;
volatile GPMI_DMA_Desc *chains_write;
volatile GPMI_DMA_Desc *chains_erase;
uint32_t *FlashSendCommandBuffer;

typedef struct GPMI_Timing_t {
    uint32_t tPROG_us;
    uint32_t tBERS_us;
    uint32_t tREAD_us;
    unsigned char DataSetup_ns;
    unsigned char DataHold_ns;
    unsigned char AddressSetup_ns;
    float SampleDelay_cyc;
} GPMI_Timing_t;

static struct GPMI_Timing_t defaultTiming =
    {
        .DataSetup_ns = 12 + 2,
        .DataHold_ns = 5 + 2,
        .AddressSetup_ns = 12 + 2,
        .SampleDelay_cyc = 0.0,
        .tREAD_us = 12,
        .tPROG_us = 12,
        .tBERS_us = 2500};

uint32_t nand_pgbuf[2048 / 4] __aligned(4);
uint32_t GPMI_AuxiliaryBuffer[512 / 4] __aligned(4);

volatile uint32_t iocnt_read = 0;
volatile uint32_t iocnt_write = 0;
volatile uint32_t iocnt_erase = 0;

void bsp_nand_init(uint32_t *dma_chain_info) {
    chains_cmd = (GPMI_DMA_Desc *)dma_chain_info[0];
    chains_read = (GPMI_DMA_Desc *)dma_chain_info[1];
    chains_write = (GPMI_DMA_Desc *)dma_chain_info[2];
    chains_erase = (GPMI_DMA_Desc *)dma_chain_info[3];
    FlashSendCommandBuffer = (uint32_t *)dma_chain_info[4];
    bsp_nand_getid();
}

void GPMI_SendCMD(uint32_t *cmd, uint16_t par_len, uint32_t *rec_buf, uint32_t readback_len) {
    assert(!((uint32_t)rec_buf % 4));
    while (HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA)
        ;
    // mmu_clean_dcache(cmd, par_len);
    chains_cmd[1].dma_cmd &= 0x0000FFFF;
    chains_cmd[1].dma_cmd |= (1 + par_len) << 16;
    chains_cmd[1].dma_bar = (uint32_t)cmd;

    chains_cmd[1].gpmi_ctrl0 &= 0xFFFF0000;
    chains_cmd[1].gpmi_ctrl0 |= ((1 + par_len) & 0xFFFF);

    if (par_len) {
        chains_cmd[1].dma_nxtcmdar = (uint32_t)&chains_cmd[2];

        chains_cmd[2].dma_cmd &= 0x0000FFFF;
        chains_cmd[2].dma_cmd |= (1 + readback_len) << 16;
        chains_cmd[2].gpmi_ctrl0 &= 0xFFFF0000;
        chains_cmd[2].gpmi_ctrl0 |= ((1 + readback_len) & 0xFFFF);
        chains_cmd[2].dma_bar = (uint32_t)PA(rec_buf);
    } else {
        chains_cmd[1].dma_nxtcmdar = (uint32_t)&chains_cmd[3];
    }

    // mmu_clean_invalidated_dcache(chains_cmd, sizeof(chains_cmd));
    // mmu_drain_buffer();
    // INFO("AD1:%08lX, AD2:%08lX\r\n",chains_cmd,&chains_cmd[0] );

    BF_WRn(APBH_CHn_NXTCMDAR, NAND_DMA_Channel, CMD_ADDR, (reg32_t)chains_cmd);
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(NAND_DMA_Channel, 1);

    while (HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA)
        ;
    while (!BF_RD(APBH_CTRL1, CH4_CMDCMPLT_IRQ))
        ;

    BF_CLR(APBH_CTRL1, CH4_CMDCMPLT_IRQ);
    BF_CLR(APBH_CTRL1, CH4_AHB_ERROR_IRQ);

    mmu_invalidate_dcache(rec_buf, readback_len);
}

uint32_t *bsp_nand_getid() {
    uint32_t buf[32];
    uint32_t *FlashRecCommandBuffer = &buf[16];
    FlashSendCommandBuffer[0] = 0x90;
    GPMI_SendCMD(FlashSendCommandBuffer, 1, FlashRecCommandBuffer, 6);
    INFO("Flash ID:\r\n");
    for (int i = 0; i < 6; i++) {
        INFO("%02x ", ((uint8_t *)FlashRecCommandBuffer)[i]);
    }
    INFO("\n");
    char DIDesc4Rd = ((uint8_t *)FlashRecCommandBuffer)[3];
    uint32_t PageSize_B = (1 << (DIDesc4Rd & 0x3)) * 1024;
    uint32_t SpareSizePerPage_B = PageSize_B / 512 * ((1 << ((DIDesc4Rd >> 2) & 1)) * 8);
    uint32_t BlockSize_KB = (1 << ((DIDesc4Rd >> 4) & 0x3)) * 64;
    uint32_t PagesPerBlock = BlockSize_KB * 1024 / PageSize_B;

    INFO("PageSize:%lu B\n", PageSize_B);
    INFO("SpareSizePerPage:%lu B\n", SpareSizePerPage_B);
    INFO("BlockSize:%lu KB\n", BlockSize_KB);
    INFO("PagesPerBlock:%lu\n", PagesPerBlock);
    page_sz = PageSize_B;
    // return FlashRecCommandBuffer;
    return NULL;
}

int static nand_read(uint32_t page, uint32_t *ECC)
{
    int ret = 0;
    volatile uint8_t *cmdBuf = (uint8_t *)FlashSendCommandBuffer;
    // assert(!(((uint32_t)buffer) % 4));
    while ((HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA))
        ;

    while (HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.AHB_BYTES)
        ;
    while (HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.APB_BYTES)
        ;
    cmdBuf[0] = NAND_CMD_READ0;
    cmdBuf[1] = 0 & 0xFF;
    cmdBuf[2] = (0 >> 8) & 0xFF;
    cmdBuf[3] = page & 0xFF;
    cmdBuf[4] = (page >> 8) & 0xFF;
    cmdBuf[5] = NAND_CMD_READSTART;
    chains_read[4].gpmi_data_ptr = (uint32_t)PA(nand_pgbuf);
    chains_read[4].gpmi_aux_ptr = (uint32_t)PA(GPMI_AuxiliaryBuffer);

    // mmu_drain_buffer();
    // mmu_clean_invalidated_dcache(chains_cmd, sizeof(chains_cmd));

    BF_WRn(APBH_CHn_NXTCMDAR, NAND_DMA_Channel, CMD_ADDR, (reg32_t)&chains_read[0]);
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(NAND_DMA_Channel, 1);

    while ((HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA))
        ;
    while (!BF_RD(APBH_CTRL1, CH4_CMDCMPLT_IRQ))
        ;
    while ((!BF_RD(ECC8_CTRL, COMPLETE_IRQ)) && (!BF_RD(ECC8_CTRL, BM_ERROR_IRQ)))
        ;
    if (BF_RD(GPMI_CTRL1, TIMEOUT_IRQ))
        INFO("GPMI_TIMEOUT_IRQ\n");
    if (BF_RD(GPMI_CTRL1, DEV_IRQ))
        INFO("GPMI_DEV_IRQ\n");

    if (BF_RDn(APBH_CHn_CURCMDAR, NAND_DMA_Channel, CMD_ADDR) == (uint32_t)&chains_read[8]) {
        ret = -1;
        INFO("GPMI_OPA_READ psense compare ERROR\n");
    }

    if (BF_RD(ECC8_CTRL, COMPLETE_IRQ)) {
        uint8_t eccs[4];
        eccs[0] =  BF_RD(ECC8_STATUS1, STATUS_PAYLOAD0);
        eccs[1] =  BF_RD(ECC8_STATUS1, STATUS_PAYLOAD1);
        eccs[2] =  BF_RD(ECC8_STATUS1, STATUS_PAYLOAD2);
        eccs[3] =  BF_RD(ECC8_STATUS1, STATUS_PAYLOAD3);
        for(int i = 0; i < 4 ; i++)
        {
            if((eccs[i] > 0 ) && (eccs[i] < 0xF))
            {
                INFO("ECC WARN:%ld:%d,%02X\r\n", page,i, eccs[i]);
            }
        }

        //if (ECC) {
        //    *ECC = BF_RD(ECC8_STATUS1, STATUS_PAYLOAD0) |
        //           (BF_RD(ECC8_STATUS1, STATUS_PAYLOAD1) << 8) |
        //           (BF_RD(ECC8_STATUS1, STATUS_PAYLOAD2) << 16) |
        //           (BF_RD(ECC8_STATUS1, STATUS_PAYLOAD3) << 24);
        //}
    }

    if (BF_RD(ECC8_CTRL, BM_ERROR_IRQ)) {
        INFO("NAND ECC ERROR! page:%ld\r\n", page);
        ret = -1;
    }

    BF_CLR(GPMI_CTRL1, DEV_IRQ);
    BF_CLR(GPMI_CTRL1, TIMEOUT_IRQ);
    BF_CLR(ECC8_CTRL, COMPLETE_IRQ);
    BF_CLR(ECC8_CTRL, BM_ERROR_IRQ);
    BF_CLR(APBH_CTRL1, CH4_CMDCMPLT_IRQ);
    BF_CLR(APBH_CTRL1, CH4_AHB_ERROR_IRQ);

    while (HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.AHB_BYTES)
        ;
    while (HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.APB_BYTES)
        ;

    bsp_delayus(defaultTiming.tREAD_us);
    iocnt_read++;
    return ret;
}

int bsp_nand_read_page_nometa(uint32_t page, uint8_t *buffer, uint32_t *ECC) {

    int ret = nand_read(page, ECC);

    mmu_invalidate_dcache(nand_pgbuf, page_sz);

    if (buffer) {
        memcpy(buffer, nand_pgbuf, page_sz);
    }
    return ret;
}

int bsp_nand_read_page(uint32_t page, uint8_t *buffer, uint32_t *ECC) {

    int ret = nand_read(page, ECC);

    mmu_invalidate_dcache(nand_pgbuf, page_sz);
    mmu_invalidate_dcache(GPMI_AuxiliaryBuffer, 19);

    if (buffer) {
        memcpy(buffer, nand_pgbuf, page_sz);
        memcpy(&buffer[page_sz], GPMI_AuxiliaryBuffer, 19);
    }
    return ret;
}

void bsp_nand_erase_block(uint32_t block) {
    volatile uint8_t *cmdBuf = (uint8_t *)FlashSendCommandBuffer;
    while ((HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA))
        ;

    while (HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.AHB_BYTES)
        ;
    while (HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.APB_BYTES)
        ;
    cmdBuf[0] = NAND_CMD_ERASE1;
    cmdBuf[1] = (block << 6) & 0xFF;
    cmdBuf[2] = (block >> 2) & 0xFF;
    cmdBuf[3] = (block >> 10) & 0xFF;
    cmdBuf[4] = NAND_CMD_ERASE2;
    cmdBuf[5] = NAND_CMD_STATUS;

    BF_WRn(APBH_CHn_NXTCMDAR, NAND_DMA_Channel, CMD_ADDR, (reg32_t)&chains_erase[0]);
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(NAND_DMA_Channel, 1);

    while ((HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA))
        ;

    while (!BF_RD(APBH_CTRL1, CH4_CMDCMPLT_IRQ))
        ;

    if (BF_RDn(APBH_CHn_CURCMDAR, NAND_DMA_Channel, CMD_ADDR) == (uint32_t)&chains_erase[8]) {
        INFO("GPMI_OPA_READ psense compare ERROR\n");
    }

    while (HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.AHB_BYTES)
        ;
    while (HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.APB_BYTES)
        ;
    bsp_delayus(defaultTiming.tBERS_us);

    BF_CLR(ECC8_CTRL, COMPLETE_IRQ);
    BF_CLR(ECC8_CTRL, BM_ERROR_IRQ);
    BF_CLR(APBH_CTRL1, CH4_CMDCMPLT_IRQ);
    BF_CLR(APBH_CTRL1, CH4_AHB_ERROR_IRQ);
    iocnt_erase++;
}

void static nand_write(uint32_t page)
{
while (HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.AHB_BYTES)
        ;
    while (HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.APB_BYTES)
        ;
    while ((HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA))
        ;
    volatile uint8_t *cmdBuf = (uint8_t *)FlashSendCommandBuffer;
    cmdBuf[0] = NAND_CMD_SEQIN;
    cmdBuf[1] = 0 & 0xFF;
    cmdBuf[2] = (0 >> 8) & 0xFF;
    cmdBuf[3] = page & 0xFF;
    cmdBuf[4] = (page >> 8) & 0xFF;
    cmdBuf[5] = NAND_CMD_PAGEPROG;
    cmdBuf[6] = NAND_CMD_STATUS;
    //if (!addr_align) {
        chains_write[1].dma_bar = (uint32_t)PA(nand_pgbuf);
    //} else {
    //    chains_write[1].dma_bar = (uint32_t)PA(dat);
    //}
    chains_write[2].dma_bar = (uint32_t)PA(GPMI_AuxiliaryBuffer);

    BF_WRn(APBH_CHn_NXTCMDAR, NAND_DMA_Channel, CMD_ADDR, (reg32_t)&chains_write[0]);
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(NAND_DMA_Channel, 1);
    bsp_delayus(10);
    // INFO("START WR 2\r\n");
    while ((HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA))
        ;

    while (!BF_RD(APBH_CTRL1, CH4_CMDCMPLT_IRQ))
        ;
    if (BF_RD(GPMI_CTRL1, TIMEOUT_IRQ))
        INFO("GPMI_TIMEOUT_IRQ\n");
    if (BF_RD(GPMI_CTRL1, DEV_IRQ))
        INFO("GPMI_DEV_IRQ\n");

    if (BF_RDn(APBH_CHn_CURCMDAR, NAND_DMA_Channel, CMD_ADDR) == (uint32_t)&chains_erase[8]) {
        INFO("GPMI_OPA_WRITE psense compare ERROR\n");
    }

    BF_CLR(GPMI_CTRL1, DEV_IRQ);
    BF_CLR(GPMI_CTRL1, TIMEOUT_IRQ);
    BF_CLR(ECC8_CTRL, COMPLETE_IRQ);
    BF_CLR(ECC8_CTRL, BM_ERROR_IRQ);
    BF_CLR(APBH_CTRL1, CH4_CMDCMPLT_IRQ);
    BF_CLR(APBH_CTRL1, CH4_AHB_ERROR_IRQ);

    // INFO("NAND WR 3\r\n");
    while (HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.AHB_BYTES)
        ;
    while (HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.APB_BYTES)
        ;

    // bsp_delayus(200);
    bsp_delayus(defaultTiming.tPROG_us);
    iocnt_write++;
}

void bsp_nand_write_page(uint32_t page, uint8_t *dat) {

    memcpy(nand_pgbuf, dat, page_sz);
    mmu_clean_dcache(nand_pgbuf, page_sz);
    memcpy(GPMI_AuxiliaryBuffer, &dat[page_sz], 19);
    mmu_clean_dcache(GPMI_AuxiliaryBuffer, 19);
    nand_write(page);
}

void bsp_nand_write_page_nometa(uint32_t page, uint8_t *dat) {

    memcpy(nand_pgbuf, dat, page_sz);
    mmu_clean_dcache(nand_pgbuf, page_sz);
    memset(GPMI_AuxiliaryBuffer, 0xFF, 19);
    mmu_clean_dcache(GPMI_AuxiliaryBuffer, 19);
    nand_write(page);
}

bool bsp_nand_check_is_bad_block(uint32_t block) {
    bsp_nand_read_page(64 * block, NULL, NULL);
    return !GPMI_AuxiliaryBuffer[0];
}
