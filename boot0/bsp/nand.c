#include "board.h"
#include "loader.h"
#include "utils.h"

#include <string.h>

#define CHIP_SEL  0
#define NAND_DMA_Channel  4
#define MASK_AND_REFERENCE_VALUE 0x0100


#define NAND_CMD_READ0      0
#define NAND_CMD_READSTART  0x30

#define NAND_CMD_ERASE1 0x60
#define NAND_CMD_ERASE2 0xd0

#define NAND_CMD_SEQIN 0x80
#define NAND_CMD_PAGEPROG 0x10

#define NAND_CMD_STATUS 0x70

typedef struct GPMI_DMA_Desc
{
    // DMA related fields
    uint32_t dma_nxtcmdar;
    uint32_t dma_cmd;
    uint32_t dma_bar;
    // GPMI related fields
    uint32_t gpmi_ctrl0;    //PIO 0
    uint32_t gpmi_compare;  //PIO 1
    uint32_t gpmi_eccctrl;  //PIO 2
    uint32_t gpmi_ecccount; //PIO 3
    uint32_t gpmi_data_ptr; //PIO 4
    uint32_t gpmi_aux_ptr;  //PIO 5
} GPMI_DMA_Desc;

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
        .tREAD_us = 50,
        .tPROG_us = 50,
        .tBERS_us = 2100
};
 

DMA_MEM_CHAINS volatile GPMI_DMA_Desc chains_cmd[4];
DMA_MEM_CHAINS volatile GPMI_DMA_Desc chains_read[8 + 1];
DMA_MEM_CHAINS volatile GPMI_DMA_Desc chains_write[10 + 1];
DMA_MEM_CHAINS volatile GPMI_DMA_Desc chains_erase[8 + 1];
DMA_MEM_CHAINS uint32_t FlashSendCommandBuffer[2];
DMA_MEM uint32_t FlashSendParaBuffer[4];
DMA_MEM uint32_t FlashRecCommandBuffer[4];

DMA_MEM uint32_t volatile GPMI_DataBuffer[ 2048 / 4 ]  ;
DMA_MEM uint32_t volatile GPMI_AuxiliaryBuffer[ 512 / 4 ]  ;

static uint32_t page_sz = 2048;

static uint64_t GPMIFreq;
static uint64_t DeviceTimeOutCycles;

void GPMI_Init()
{
    
    BF_CLR(CLKCTRL_GPMI, CLKGATE);
    
    BF_CLR(GPMI_CTRL0, SFTRST);
    BF_CLR(GPMI_CTRL0, CLKGATE);

    BF_SET(GPMI_CTRL0, SFTRST);
    
    while(BF_RD(GPMI_CTRL0, CLKGATE) == 0){
        ;
    }

    BF_CLR(GPMI_CTRL0, SFTRST);
    BF_CLR(GPMI_CTRL0, CLKGATE);

    BF_CS8(
        PINCTRL_MUXSEL0,
        BANK0_PIN07, 0, //D7
        BANK0_PIN06, 0, //D6
        BANK0_PIN05, 0, //D5
        BANK0_PIN04, 0, //D4
        BANK0_PIN03, 0, //D3
        BANK0_PIN02, 0, //D2
        BANK0_PIN01, 0, //D1
        BANK0_PIN00, 0  //D0
    );

    BF_CS1(
        PINCTRL_MUXSEL4,
        BANK2_PIN15, 1 //GPMI_CE0N
    );

    BF_CS7(
        PINCTRL_MUXSEL1,
        BANK0_PIN25, 0, //RDn
        BANK0_PIN24, 0, //WRn
        BANK0_PIN23, 3, //IRQ
        BANK0_PIN22, 0, //RSTn
        BANK0_PIN19, 0, //RB0

        BANK0_PIN17, 0, //A1
        BANK0_PIN16, 0  //A0
    );

    BF_CS8(
        PINCTRL_DRIVE0,
        BANK0_PIN07_MA, 2,      //12mA
        BANK0_PIN06_MA, 2,      //12mA
        BANK0_PIN05_MA, 2,      //12mA
        BANK0_PIN04_MA, 2,      //12mA
        BANK0_PIN03_MA, 2,      //12mA
        BANK0_PIN02_MA, 2,      //12mA
        BANK0_PIN01_MA, 2,      //12mA
        BANK0_PIN00_MA, 2       //12mA
    );

    BF_CS5(
        PINCTRL_DRIVE2,
        BANK0_PIN23_MA, 2,      //12mA
        BANK0_PIN22_MA, 2,      //12mA
        BANK0_PIN19_MA, 2,      //12mA
        BANK0_PIN17_MA, 2,      //12mA
        BANK0_PIN16_MA, 2       //12mA
    );

    BF_CS2(
        PINCTRL_DRIVE3,
        BANK0_PIN25_MA, 2,      //12mA
        BANK0_PIN24_MA, 2      //12mA
    );

}

void HardECC8_Init()
{
    BF_CLR(ECC8_CTRL, SFTRST);
    BF_CLR(ECC8_CTRL, CLKGATE);

    BF_SET(ECC8_CTRL, SFTRST);
    while(BF_RD(ECC8_CTRL, CLKGATE) == 0)
    {
        ;
    }

    BF_CLR(ECC8_CTRL, SFTRST);
    BF_CLR(ECC8_CTRL, CLKGATE);

    BF_CLR(ECC8_CTRL, AHBM_SFTRST);
    
}



void GPMI_SetAccessTiming(GPMI_Timing_t timing)
{

    DeviceTimeOutCycles = nsToCycles(80000000, 1000000000UL / (GPMIFreq / 4096UL), 0);  //80ms
    //DeviceTimeOutCycles = 0xFFFF;

    BF_CS3(
        GPMI_TIMING0,
        DATA_SETUP, nsToCycles(timing.DataSetup_ns, 1000000000UL / GPMIFreq, 1),
        DATA_HOLD, nsToCycles(timing.DataHold_ns, 1000000000UL / GPMIFreq, 1),
        ADDRESS_SETUP, nsToCycles(timing.DataSetup_ns, 1000000000UL / GPMIFreq, 0)
    );
        
    BF_CS1(
        GPMI_TIMING1,
        DEVICE_BUSY_TIMEOUT, DeviceTimeOutCycles);

    BF_CS2(
        GPMI_CTRL1,
        DSAMPLE_TIME, (uint32_t)(timing.SampleDelay_cyc * 2),
        BURST_EN, 1);


    INFO("DATA_SETUP:%02X\r\n", BF_RD(GPMI_TIMING0, DATA_SETUP));
    INFO("DATA_HOLD:%02X\r\n", BF_RD(GPMI_TIMING0, DATA_HOLD));
    INFO("ADDRESS_SETUP:%02X\r\n", BF_RD(GPMI_TIMING0, ADDRESS_SETUP));
    INFO("DSAMPLE_TIME:%02X\r\n", BF_RD(GPMI_CTRL1, DSAMPLE_TIME));

}


void interface_init()
{
    GPMI_Init();
    HardECC8_Init();

    BF_CLR(CLKCTRL_GPMI, CLKGATE);
    BF_CLR(CLKCTRL_FRAC, CLKGATEIO);

    HW_CLKCTRL_GPMI_WR(BF_CLKCTRL_GPMI_DIV(8));    // 480 / 4 MHz
    INFO("GPMI CLK DIV:%d\n", BF_RD(CLKCTRL_GPMI, DIV));
    //BF_CS1(CLKCTRL_GPMI, DIV, 2);

    BF_CLR(CLKCTRL_CLKSEQ, BYPASS_GPMI);    //Set TO HF

    GPMIFreq = 480000000UL / 8UL;
    //GPMIFreq = 24000000UL;

    BF_CLRV(APBH_CTRL0, CLKGATE_CHANNEL, 0x10);
    BF_SETV(APBH_CTRL0, RESET_CHANNEL, 0x10);

    BF_CS1(GPMI_CTRL1, DEV_RESET, BV_GPMI_CTRL1_DEV_RESET__DISABLED);
    BF_CS1(GPMI_CTRL1, ATA_IRQRDY_POLARITY, BV_GPMI_CTRL1_ATA_IRQRDY_POLARITY__ACTIVEHIGH);
    BF_CS1(GPMI_CTRL1, GPMI_MODE, BV_GPMI_CTRL1_GPMI_MODE__NAND);

    GPMI_SetAccessTiming(defaultTiming);


    BF_SET(APBH_CTRL1, CH4_CMDCMPLT_IRQ_EN);
    BF_SET(ECC8_CTRL, COMPLETE_IRQ_EN);
    BF_SET(GPMI_CTRL0, DEV_IRQ_EN);
    BF_SET(GPMI_CTRL0, TIMEOUT_IRQ_EN);


    BF_CLR(GPMI_CTRL1, DEV_IRQ);
    BF_CLR(GPMI_CTRL1, TIMEOUT_IRQ);

    BF_CLR(APBH_CTRL1, CH4_CMDCMPLT_IRQ);
    BF_CLR(APBH_CTRL1, CH4_AHB_ERROR_IRQ);

    BF_CLR(ECC8_CTRL, COMPLETE_IRQ);
    BF_CLR(ECC8_CTRL, BM_ERROR_IRQ);
}


static void GPMI_DMAChainsInit()
{
    // ============================================Command Chains================================================
    //  Phase 1: Wait for ready;
    chains_cmd[0].dma_cmd       =   BF_APBH_CHn_CMD_CMDWORDS(1)         |          // 发送1个PIO命令到GPMI控制器
                                    BF_APBH_CHn_CMD_WAIT4ENDCMD(1)      |          // 完成当前命令之后再继续执行
                                    BF_APBH_CHn_CMD_NANDWAIT4READY(1)   |          // 等待NAND就绪后开始执行
                                    BF_APBH_CHn_CMD_NANDLOCK(0)         |          // 锁住NAND防止被其它DMA通道占用
                                    BF_APBH_CHn_CMD_CHAIN(1)            |          // 还有剩下的描述符链
                                    BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);

    chains_cmd[0].dma_bar       =   0;

    chains_cmd[0].gpmi_ctrl0    =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY)            | // 当前模式：等待NAND就绪
                                    BF_GPMI_CTRL0_TIMEOUT_IRQ_EN(0)             |
                                    BF_GPMI_CTRL0_WORD_LENGTH(BV_GPMI_CTRL0_WORD_LENGTH__8_BIT) | // 8bit总线模式
                                    BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA)                      | // 数据模式
                                    BF_GPMI_CTRL0_CS(CHIP_SEL);                                   // 片选

    chains_cmd[0].dma_nxtcmdar  =   (uint32_t)&chains_cmd[1];

    //  Phase 2: Send command and address; Lock the nand flash
    chains_cmd[1].dma_cmd       =   BF_APBH_CHn_CMD_XFER_COUNT(1 + 00000000)    | // 1字节命令 和剩下的地址数据
                                    BF_APBH_CHn_CMD_CMDWORDS(3)                 | // 发送3个PIO命令到GPMI控制器
                                    BF_APBH_CHn_CMD_WAIT4ENDCMD(1)              | // 等待NAND就绪后开始执行
                                    BF_APBH_CHn_CMD_SEMAPHORE(0)                |
                                    BF_APBH_CHn_CMD_NANDWAIT4READY(0)           |
                                    BF_APBH_CHn_CMD_NANDLOCK(1)                 |
                                    BF_APBH_CHn_CMD_IRQONCMPLT(0)               |
                                    BF_APBH_CHn_CMD_CHAIN(1)                    |
                                    BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);      // 从内存读取，发送到NAND

    chains_cmd[1].dma_bar       =   00000000;

    chains_cmd[1].gpmi_ctrl0    =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE)     | // 写入NAND
                                    BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)      |
                                    BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED)       |
                                    BF_GPMI_CTRL0_TIMEOUT_IRQ_EN(0)             |
                                    BF_GPMI_CTRL0_CS(CHIP_SEL)                  |
                                    BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE)       |
                                    BF_GPMI_CTRL0_ADDRESS_INCREMENT(1)          | // 1 byte CMD before multi-byte ADDRESS
                                    BF_GPMI_CTRL0_XFER_COUNT(1 + 00000000);
    
    chains_cmd[1].gpmi_compare = 0;
    chains_cmd[1].gpmi_eccctrl = 0;

    chains_cmd[1].dma_nxtcmdar  =   (uint32_t)&chains_cmd[2];   //
    //  Phase 3: Readback command result;    
    chains_cmd[2].dma_cmd       =   BF_APBH_CHn_CMD_XFER_COUNT(00000000)        |  //Readback bytes
                                    BF_APBH_CHn_CMD_CMDWORDS(3)                 |
                                    BF_APBH_CHn_CMD_WAIT4ENDCMD(0)              |
                                    BF_APBH_CHn_CMD_SEMAPHORE(0)                |
                                    BF_APBH_CHn_CMD_NANDWAIT4READY(0)           |
                                    BF_APBH_CHn_CMD_NANDLOCK(0)                 |
                                    BF_APBH_CHn_CMD_IRQONCMPLT(0)               |
                                    BF_APBH_CHn_CMD_CHAIN(1)                    |
                                    BV_FLD(APBH_CHn_CMD, COMMAND, DMA_WRITE);

    chains_cmd[2].dma_bar       =   00000000;    //readback address

    chains_cmd[2].gpmi_ctrl0    =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ)      |
                                    BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)      |
                                    BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED)       |
                                    BF_GPMI_CTRL0_TIMEOUT_IRQ_EN(0)             |
                                    BF_GPMI_CTRL0_CS(CHIP_SEL)                  |
                                    BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA)      |
                                    BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)          |
                                    BF_GPMI_CTRL0_XFER_COUNT(00000000);             //Readback bytes

    chains_cmd[2].gpmi_compare = 0;
    chains_cmd[2].gpmi_eccctrl = 0;

    chains_cmd[2].dma_nxtcmdar = (reg32_t)&chains_cmd[3];

    //  Phase 4: Terminate;  

    chains_cmd[3].dma_cmd      =    BF_APBH_CHn_CMD_IRQONCMPLT(1)                   |
                                    BF_APBH_CHn_CMD_WAIT4ENDCMD(1)                  |
                                    BF_APBH_CHn_CMD_SEMAPHORE(1)                    |  
                                    BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);

    chains_cmd[3].dma_bar = 0;
    chains_cmd[3].dma_nxtcmdar = 0;

    // ============================================Read Chains================================================
    //  Phase 1: issue NAND read setup command (CLE/ALE);
    chains_read[0].dma_cmd          =   BF_APBH_CHn_CMD_XFER_COUNT(1 + 4)   |       // point to the next descriptor
                                        BF_APBH_CHn_CMD_CMDWORDS(3)         |       // send 3 words to the GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)      |       // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)        |       
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)   |       
                                        BF_APBH_CHn_CMD_NANDLOCK(1)         |       
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)       |       
                                        BF_APBH_CHn_CMD_CHAIN(1)            |       // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);    // read data from DMA, write to NAND

    chains_read[0].dma_bar          =   (uint32_t)FlashSendCommandBuffer;

    chains_read[0].gpmi_ctrl0       =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)  |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED)    |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)              |
                                        BF_GPMI_CTRL0_TIMEOUT_IRQ_EN(1)             |
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE)   |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(1)      |
                                        BF_GPMI_CTRL0_XFER_COUNT(1 + 4);            

    chains_read[0].gpmi_compare     =   0;

    chains_read[0].gpmi_eccctrl     =   BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE); // disable the ECC

    chains_read[0].dma_nxtcmdar     =   (uint32_t)&chains_read[1];

    //  Phase 2: issue NAND read execute command (CLE);
    
    chains_read[1].dma_cmd          =   BF_APBH_CHn_CMD_XFER_COUNT(1)       |       // 1 byte read command
                                        BF_APBH_CHn_CMD_CMDWORDS(1)         |       // send 1 word to GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)      |       // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)        |       
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)   |       
                                        BF_APBH_CHn_CMD_NANDLOCK(1)         |       // prevent other DMA channels from taking over
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)       |       
                                        BF_APBH_CHn_CMD_CHAIN(1)            |       // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);    // read data from DMA, write to NAND

    chains_read[1].dma_bar          =   ((uint32_t)FlashSendCommandBuffer) + 5;     // point to byte 6, read execute command

    chains_read[1].gpmi_ctrl0       =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE)     |   // write to the NAND
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)      |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED)       |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)                  |   // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE)       |
                                        BF_GPMI_CTRL0_TIMEOUT_IRQ_EN(1)             |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)          |
                                        BF_GPMI_CTRL0_XFER_COUNT(1);                      // 1 byte command

    chains_read[1].dma_nxtcmdar     =   (uint32_t)&chains_read[2];                // point to the next descriptor


    //  Phase 3: wait for ready (DATA);
    chains_read[2].dma_cmd          =   BF_APBH_CHn_CMD_XFER_COUNT(0)       |       // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(1)         |       // send 1 word to GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)      |       // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)        |       
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(1)   |       // wait for nand to be ready
                                        BF_APBH_CHn_CMD_NANDLOCK(1)         |       // relinquish nand lock
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)       |       
                                        BF_APBH_CHn_CMD_CHAIN(1)            |       // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER); // no dma transfer

    chains_read[2].dma_bar          =   0; // field not used
            // 1 word sent to the GPMI
    chains_read[2].gpmi_ctrl0       =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY)    | // wait for NAND ready
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)              |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED)               |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)                          | // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA)              |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)                  |
                                        BF_GPMI_CTRL0_TIMEOUT_IRQ_EN(1)             |
                                        BF_GPMI_CTRL0_XFER_COUNT(0);

    chains_read[2].dma_nxtcmdar     =   (uint32_t)&chains_read[3];    // point to the next descriptor


    //  Phase 4: psense compare (time out check);
    chains_read[3].dma_cmd          =   BF_APBH_CHn_CMD_XFER_COUNT(0)               |   // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(0)                 |   // no words sent to GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(0)              |   // do not wait to continue
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)                |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)           |
                                        BF_APBH_CHn_CMD_NANDLOCK(1)                 |
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)               |
                                        BF_APBH_CHn_CMD_CHAIN(1)                    |   // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE);       // perform a sense check

    chains_read[3].dma_bar          =   (uint32_t)&chains_read[8];                      // if sense check fails, branch to error handler

    chains_read[3].dma_nxtcmdar     =   (uint32_t)&chains_read[4];                      // point to the next descriptor



    //  Phase 5: read 2K page plus 19 byte meta-data Nand data and send it to ECC block (DATA);
    chains_read[4].dma_cmd          =   BF_APBH_CHn_CMD_XFER_COUNT(0)       |           // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(6)         |           // send 6 words to GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)      |           // wait for command to finish beforecontinuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)        |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)   |
                                        BF_APBH_CHn_CMD_NANDLOCK(1)         |           // prevent other DMA channels from taking over
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)       |           // ECC block generates ecc8 interrupt on completion
                                        BF_APBH_CHn_CMD_CHAIN(1)            |           // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);     // no DMA transfer, ECC block handlestransfer

    chains_read[4].dma_bar          =   0;                                              // field not used
    // 6 words sent to the GPMI
    chains_read[4].gpmi_ctrl0       =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ)      |   // read from the NAND
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)      |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED)       |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)                  |   // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA)      |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)          |
                                        BF_GPMI_CTRL0_XFER_COUNT(4 * (512 + 9) + (19 + 9)); 
                                        // 2K PAGE SIZE four 512 byte data blocks (plusparity, t = 4) 
                                        // and one 19 byte aux block (plusparity, t = 4)

    chains_read[4].gpmi_compare     =   0; // field not used but necessary to seteccctrl

    // GPMI ECCCTRL PIO This launches the 2K byte transfer through ECC8’s
    // bus master. Setting the ECC_ENABLE bit redirects the data flow
    // within the GPMI so that read data flows to the ECC8 engine instead
    // of flowing to the GPMI’s DMA channel.

    chains_read[4].gpmi_eccctrl     =   BV_FLD(GPMI_ECCCTRL, ECC_CMD, DECODE_4_BIT) |   // specify t = 4 mode
                                        BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE)    |   // enable ECC module
                                        BF_GPMI_ECCCTRL_BUFFER_MASK(0X10F);             // read all 4 data blocks and 1 aux block
    
    chains_read[4].gpmi_ecccount    =   BF_GPMI_ECCCOUNT_COUNT(4 * (512 + 9) + (19 + 9)); 
                                        // 2K PAGE SIZE specify number of bytes read from NAND

    chains_read[4].gpmi_aux_ptr     =   (uint32_t)GPMI_AuxiliaryBuffer; 
                                        // pointer for the 19 byte aux area + parity and syndrome bytes 
	                                    // for both data and aux blocks.

    chains_read[4].gpmi_data_ptr    =   00000000;  

    chains_read[4].dma_nxtcmdar     =   (uint32_t)&chains_read[5];                  // point to the next descriptor

    //  Phase 6: disable ECC block;
    chains_read[5].dma_cmd          =   BF_APBH_CHn_CMD_XFER_COUNT(0)               |   // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(3)                 |   // send 3 words to GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)              |   // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)                |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(1)           |   // wait for nand to be ready
                                        BF_APBH_CHn_CMD_NANDLOCK(1)                 |   // need nand lock to be thread safe while turnoff ECC8
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)               |
                                        BF_APBH_CHn_CMD_CHAIN(1)                    |   // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);     // no dma transfer
    chains_read[5].dma_bar          =   0;                                              // field not used
    
    chains_read[5].gpmi_ctrl0       =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY)    |
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)              |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED)               |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)                          | // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA)              |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)                  |
                                        
                                        BF_GPMI_CTRL0_XFER_COUNT(0);

    chains_read[5].gpmi_compare     =   0;                                          // field not used but necessary to set eccctrl
    chains_read[5].gpmi_eccctrl     =   BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);  // disable the ECC block

    chains_read[5].dma_nxtcmdar     =   (uint32_t)&chains_read[6];                      // point to the next descriptor

    
    //  Phase 7: deassert nand lock;
    chains_read[6].dma_cmd          =   BF_APBH_CHn_CMD_XFER_COUNT(0)               |       // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(0)                 |       // no words sent to GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)              |       // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)                |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)           |
                                        BF_APBH_CHn_CMD_NANDLOCK(0)                 |       // relinquish nand lock
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)               |       // ECC8 engine generates interrupt
                                        BF_APBH_CHn_CMD_CHAIN(1)                    |       // terminate DMA chain processing
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);         // no dma transfer
    chains_read[6].dma_bar          =   0;                                  // field not used
    chains_read[6].dma_nxtcmdar     =   (uint32_t)&chains_read[7];                      


    //  Phase 8: Terminate;
    chains_read[7].dma_cmd          =   BF_APBH_CHn_CMD_IRQONCMPLT(1)               |
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(0)              |
                                        BF_APBH_CHn_CMD_SEMAPHORE(1)                |
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);

    chains_read[7].dma_bar          =   0;
    chains_read[7].dma_nxtcmdar     =   0;

    //  ERROR Brunch;
    chains_read[8].dma_cmd          =   BF_APBH_CHn_CMD_IRQONCMPLT(1)               |
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(0)              |
                                        BF_APBH_CHn_CMD_SEMAPHORE(1)                |
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);

    chains_read[8].dma_bar          =   0;
    chains_read[8].dma_nxtcmdar     =   0;




    // ============================================Write Chains================================================
    //  Phase 1: issue NAND write setup command (CLE/ALE);

    chains_write[0].dma_nxtcmdar    =   (uint32_t)&chains_write[1];
    chains_write[0].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(1 + 4)       |       // 1 byte command, 4 byte address
                                        BF_APBH_CHn_CMD_CMDWORDS(3)             |       // send 3 words to the GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)          |       // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)            |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)       |
                                        BF_APBH_CHn_CMD_NANDLOCK(1)             |       // prevent other DMA channels from taking over
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)           |
                                        BF_APBH_CHn_CMD_CHAIN(1)                |       // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);        // read data from DMA, write to NAND

    chains_write[0].dma_bar         =   (uint32_t)FlashSendCommandBuffer;               // byte 0 write setup, bytes 1 - 4 NAND address
    chains_write[0].gpmi_ctrl0      =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |       // write to the NAND
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)  |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED)    |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)              |       // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE)   |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(1)      |       // send command and address
                                        BF_GPMI_CTRL0_XFER_COUNT(1 + 4);                // 1 byte command, 4 byte address

    chains_write[0].gpmi_compare    =   0;                             // field not used but necessary to set eccctrl
    chains_write[0].gpmi_eccctrl    =   BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);      // disable the ECC block


    //  Phase 2: write the data payload (DATA)
    
    chains_write[1].dma_nxtcmdar    =   (uint32_t)&chains_write[2];                 // point to the next descriptor
    chains_write[1].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(4 * 512)     |       // NOTE: DMA transfer only the data payload
                                        BF_APBH_CHn_CMD_CMDWORDS(4)             |       // send 4 words to the GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(0)          |       // DON’T wait to end, wait in the next descriptor
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)            |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)       |
                                        BF_APBH_CHn_CMD_NANDLOCK(1)             |       // maintain resource lock
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)           |
                                        BF_APBH_CHn_CMD_CHAIN(1)                |       // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);        // read data from DMA, write to NAND

    chains_write[1].dma_bar         =   00000000;                               //DATA               

    chains_write[1].gpmi_ctrl0      =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE) |       // write to the NAND
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)  |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED)    |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)              |       // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA)  |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)      |
                                        BF_GPMI_CTRL0_XFER_COUNT(4 * 512 + 19);         
                                        // NOTE: this field contains the total amount                      
                                        // DMA transferred (4 data and 1 aux blocks)
                                        // to GPMI
    chains_write[1].gpmi_compare    =   0;                                              // field not used but necessary to set eccctrl
    chains_write[1].gpmi_eccctrl    =   BV_FLD(GPMI_ECCCTRL, ECC_CMD, ENCODE_4_BIT) |   // specify t = 4 mode
                                        BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, ENABLE)    |   // enable ECC module
                                        BF_GPMI_ECCCTRL_BUFFER_MASK(0x10F);             // write all 8 data blocks and 1 aux block
                                                                                        
                                                                                        
    chains_write[1].gpmi_ecccount   =   BF_GPMI_ECCCOUNT_COUNT(4 * (512 + 9) + (19 + 9)); // specify number of bytes written to NAND
    // NOTE: the extra 4*(9)+9 bytes are parity
    // bytes generated by the ECC block.



    // Phase 3: write the aux payload (DATA)
    
    chains_write[2].dma_nxtcmdar    =   (uint32_t)&chains_write[3];             // point to the next descriptor
    chains_write[2].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(19)      |       // NOTE: DMA transfer only the aux block
                                        BF_APBH_CHn_CMD_CMDWORDS(0)         |       // no words sent to GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)      |       // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)        |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)   |
                                        BF_APBH_CHn_CMD_NANDLOCK(1)         |       // maintain resource lock
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)       |
                                        BF_APBH_CHn_CMD_CHAIN(1)            |       // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);    // read data from DMA, write to NAND

    chains_write[2].dma_bar         =   00000000;                               //PAYLOAD DATA   

    // Phase 4: issue NAND write execute command (CLE)
    
    chains_write[3].dma_nxtcmdar    =   (uint32_t)&chains_write[4];                 // point to the next descriptor
    chains_write[3].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(1)       |           // 1 byte command
                                        BF_APBH_CHn_CMD_CMDWORDS(3)         |           // send 3 words to the GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)      |           // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)        |   
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)   |   
                                        BF_APBH_CHn_CMD_NANDLOCK(1)         |           // maintain resource lock
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)       |   
                                        BF_APBH_CHn_CMD_CHAIN(1)            |           // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);        // read data from DMA, write to NAND
    chains_write[3].dma_bar         =   ((uint32_t)FlashSendCommandBuffer) + 5;         // point to byte 6, write execute command
                                                                                        // 3 words sent to the GPMI

    chains_write[3].gpmi_ctrl0      =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE)     |   // write to the NAND
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)      |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED)        |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)                  |   // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE)       |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)          |
                                        BF_GPMI_CTRL0_XFER_COUNT(1);                    // 1 byte command

    chains_write[3].gpmi_compare    =   0;                                              // field not used but necessary to set eccctrl
    chains_write[3].gpmi_eccctrl    =   BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);      // disable the ECC block


    // Phase 5: wait for ready (CLE);
    chains_write[4].dma_nxtcmdar    =   (uint32_t)&chains_write[5];                 // point to the next descriptor
    chains_write[4].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(0)           |       // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(1)             |       // send 1 word to the GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)          |       // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)            |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(1)       |       // wait for nand to be ready
                                        BF_APBH_CHn_CMD_NANDLOCK(0)             |       // relinquish nand lock
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)           |
                                        BF_APBH_CHn_CMD_CHAIN(1)                |       // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);     // no dma transfer
    chains_write[4].dma_bar         =   0;                         // field not used

    // 1 word sent to the GPMI
    chains_write[4].gpmi_ctrl0      =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY)    | // wait for NAND ready
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)              |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED)               |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)                          | // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA)              |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)                  |
                                        BF_GPMI_CTRL0_XFER_COUNT(0);


    // Phase 6: psense compare (time out check)
    chains_write[5].dma_nxtcmdar    =   (uint32_t)&chains_write[6];                     // point to the next descriptor
    chains_write[5].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(0)       |           // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(0)         |           // no words sent to GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(0)      |           // do not wait to continue
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)        |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)   |
                                        BF_APBH_CHn_CMD_NANDLOCK(0)         |
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)       |
                                        BF_APBH_CHn_CMD_CHAIN(1)            |           // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE);       // perform a sense check
    chains_write[5].dma_bar = (uint32_t)&chains_write[10];          // if sense check fails, branch to error handler
    

    // Phase 7: issue NAND status command (CLE)
    chains_write[6].dma_nxtcmdar    =   (uint32_t)&chains_write[7];                 // point to the next descriptor
    chains_write[6].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(1)       |       // 1 byte command
                                        BF_APBH_CHn_CMD_CMDWORDS(3)         |       // send 3 words to the GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)      |       // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)        |   
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)   |   
                                        BF_APBH_CHn_CMD_NANDLOCK(1)         |       // prevent other DMA channels from taking over
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)       |   
                                        BF_APBH_CHn_CMD_CHAIN(1)            |       // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);    // read data from DMA, write to NAND

    chains_write[6].dma_bar = ((uint32_t)FlashSendCommandBuffer) + 6; // point to byte 6, status command
    chains_write[6].gpmi_compare    =   0;                                              // field not used but necessary to set eccctrl
    chains_write[6].gpmi_eccctrl    =   BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);      // disable the ECC block
                                                                                        // 3 words sent to the GPMI
    chains_write[6].gpmi_ctrl0      =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE)     |   // write to the NAND
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)      |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED)        |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)                  |   // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE)       |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)          |
                                        BF_GPMI_CTRL0_XFER_COUNT(1);                    // 1 byte command


    // Phase 8: read status and compare (DATA);
    chains_write[7].dma_nxtcmdar    =   (uint32_t)&chains_write[8];                     // point to the next descriptor
    chains_write[7].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(0)       |           // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(2)         |           // send 2 words to the GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)      |           // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)        |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)   |
                                        BF_APBH_CHn_CMD_NANDLOCK(1)         |           // maintain resource lock
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)       |
                                        BF_APBH_CHn_CMD_CHAIN(1)            |           // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);     // no dma transfer
    chains_write[7].dma_bar         =   0;                                  // field not used
    // 2 word sent to the GPMI
    chains_write[7].gpmi_ctrl0      =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ_AND_COMPARE)  |   // read from the NAND and compare to expect
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)              |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED)               |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)                          | // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA)              |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)                  |
                                        BF_GPMI_CTRL0_XFER_COUNT(1);

    chains_write[7].gpmi_compare    =   MASK_AND_REFERENCE_VALUE;   // NOTE: mask and reference values are NAND
                                                                    //       SPECIFIC to evaluate the NAND status

    // Phase 9: psense compare (time out check);
    chains_write[8].dma_nxtcmdar    =   (uint32_t)&chains_write[9];                 // point to the next descriptor
    chains_write[8].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(0)       |       // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(0)         |       // no words sent to GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(0)      |       // do not wait to continue
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)        |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)   |
                                        BF_APBH_CHn_CMD_NANDLOCK(0)         |       // relinquish nand lock
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)       |
                                        BF_APBH_CHn_CMD_CHAIN(1)            |       // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE);   // perform a sense check
    chains_write[8].dma_bar         =   (uint32_t)&chains_write[10];                // if sense check fails, branch to error handler


    // Phase 10: emit GPMI interrupt
    chains_write[9].dma_nxtcmdar    =   0;          // not used since this is last descriptor
    chains_write[9].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(0)               |   // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(0)                 |   // no words sent to GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(0)              |   // do not wait to continue
                                        BF_APBH_CHn_CMD_SEMAPHORE(1)                |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)           |
                                        BF_APBH_CHn_CMD_NANDLOCK(0)                 |
                                        BF_APBH_CHn_CMD_IRQONCMPLT(1)               |   // emit GPMI interrupt
                                        BF_APBH_CHn_CMD_CHAIN(0)                    |   // terminate DMA chain processing
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);     // no dma transfer


    //  ERROR Brunch;
    chains_write[10].dma_cmd          =   BF_APBH_CHn_CMD_IRQONCMPLT(1)               |
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(0)              |
                                        BF_APBH_CHn_CMD_SEMAPHORE(1)                |
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);

    chains_write[10].dma_bar          =   0;
    chains_write[10].dma_nxtcmdar     =   0;

    // ============================================Erase Chains================================================
    //  Phase 1: issue NAND erase setup command (CLE/ALE);
    chains_erase[0].dma_nxtcmdar    =   (uint32_t)&chains_erase[1];
    chains_erase[0].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(1 + 3)           |   // 1 byte command, 3 byte block address
                                        BF_APBH_CHn_CMD_CMDWORDS(3)                 |   // send 3 words to the GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)              |   // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)                |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)           |
                                        BF_APBH_CHn_CMD_NANDLOCK(1)                 |   // prevent other DMA channels from taking over
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)               |
                                        BF_APBH_CHn_CMD_CHAIN(1)                    |   // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);        // read data from DMA, write to NAND

    chains_erase[0].dma_bar         =   ((uint32_t)FlashSendCommandBuffer) + 0;       // byte 0 write setup, bytes 1 - 3 NAND address
                                                                                        // 3 words sent to the GPMI
    chains_erase[0].gpmi_ctrl0      =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE)     |   // write to the NAND
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)      |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED)        |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)                  |   // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE)       |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(1)          |   // send command and address
                                        BF_GPMI_CTRL0_XFER_COUNT(1 + 3);                // 1 byte command, 2 byte address
    chains_erase[0].gpmi_compare    =   (uint32_t)NULL;                             // field not used but necessary to set eccctrl
    chains_erase[0].gpmi_eccctrl    =   BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);      // disable the ECC block


    //  Phase 2: issue NAND erase conform command (CLE/ALE);


    chains_erase[1].dma_nxtcmdar    =   (uint32_t)&chains_erase[2];                 // point to the next descriptor
    chains_erase[1].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(1)               |   // 1 byte command
                                        BF_APBH_CHn_CMD_CMDWORDS(3)                 |   // send 3 words to the GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)              |   // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)                |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)           |
                                        BF_APBH_CHn_CMD_NANDLOCK(1)                 |   // maintain resource lock
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)               |
                                        BF_APBH_CHn_CMD_CHAIN(1)                    |   // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);        // read data from DMA, write to NAND
    chains_erase[1].dma_bar         =   ((uint32_t)FlashSendCommandBuffer) + 4;       // point to byte 4, write execute command
                                                                                        // 3 words sent to the GPMI
    chains_erase[1].gpmi_ctrl0      =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE)     |   // write to the NAND
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)      |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED)        |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)                  |   // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE)       |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)          |
                                        BF_GPMI_CTRL0_XFER_COUNT(1);                    // 1 byte command
    chains_erase[1].gpmi_compare    =   (uint32_t)NULL;                             // field not used but necessary to set eccctrl
    chains_erase[1].gpmi_eccctrl    =   BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);      // disable the ECC block



    //  Phase 3: wait for ready (CLE)
    chains_erase[2].dma_nxtcmdar    =   (uint32_t)&chains_erase[3];                 // point to the next descriptor
    chains_erase[2].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(0)               |   // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(1)                 |   // send 1 word to the GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)              |   // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)                |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(1)           |   // wait for nand to be ready
                                        BF_APBH_CHn_CMD_NANDLOCK(0)                 |   // relinquish nand lock
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)               |
                                        BF_APBH_CHn_CMD_CHAIN(1)                    |   // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);     // no dma transfer
    chains_erase[2].dma_bar         =   (uint32_t)NULL;                             // field not used

    // 1 word sent to the GPMI
    chains_erase[2].gpmi_ctrl0      =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WAIT_FOR_READY)    |   // wait for NAND ready
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)              |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED)               |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)                          |   // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA)              |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)                  |
                                        BF_GPMI_CTRL0_XFER_COUNT(0);


    //  Phase 4: psense compare (time out check)
    chains_erase[3].dma_nxtcmdar    =   (uint32_t)&chains_erase[4];                         // point to the next descriptor
    chains_erase[3].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(0)                       |   // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(0)                         |   // no words sent to GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(0)                      |   // do not wait to continue
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)                        |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)                   |
                                        BF_APBH_CHn_CMD_NANDLOCK(0)                         |
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)                       |
                                        BF_APBH_CHn_CMD_CHAIN(1)                            |   // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE);               // perform a sense check
    
    chains_erase[3].dma_bar         =   (uint32_t)&chains_erase[8];            // if sense check fails, branch to error handler


    //  Phase 5: setup read flash status command

    chains_erase[4].dma_nxtcmdar    =   (uint32_t)&chains_erase[5];                         // point to the next descriptor
    chains_erase[4].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(1)                       |   // 1 byte command
                                        BF_APBH_CHn_CMD_CMDWORDS(3)                         |   // send 3 words to the GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)                      |   // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)                        |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)                   |
                                        BF_APBH_CHn_CMD_NANDLOCK(1)                         |   // prevent other DMA channels from taking over
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)                       |
                                        BF_APBH_CHn_CMD_CHAIN(1)                            |   // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);                // read data from DMA, write to NAND
    chains_erase[4].dma_bar         =   ((uint32_t)FlashSendCommandBuffer) + 5;               // status command
    chains_erase[4].gpmi_compare    =   (uint32_t)NULL;                                     // field not used but necessary to set eccctrl
    chains_erase[4].gpmi_eccctrl    =   BV_FLD(GPMI_ECCCTRL, ENABLE_ECC, DISABLE);              // disable the ECC block
                                                                                                // 3 words sent to the GPMI
    chains_erase[4].gpmi_ctrl0      =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, WRITE)             |   // write to the NAND
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)              |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, ENABLED)                |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)                          |   // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_CLE)               |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)                  |
                                        BF_GPMI_CTRL0_XFER_COUNT(1);                            // 1 byte command

    //  Phase 6: read status and compare (DATA)

    chains_erase[5].dma_nxtcmdar    =   (uint32_t)&chains_erase[6];                         // point to the next descriptor
    chains_erase[5].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(0)                       |   // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(2)                         |   // send 2 words to the GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(1)                      |   // wait for command to finish before continuing
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)                        |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)                   |
                                        BF_APBH_CHn_CMD_NANDLOCK(1)                         |   // maintain resource lock
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)                       |
                                        BF_APBH_CHn_CMD_CHAIN(1)                            |   // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);             // no dma transfer
    chains_erase[5].dma_bar         =   (uint32_t)NULL;                                     // field not used
    // 2 word sent to the GPMI
    chains_erase[5].gpmi_ctrl0      =   BV_FLD(GPMI_CTRL0, COMMAND_MODE, READ_AND_COMPARE)  |   // read from the NAND and compare to expect
                                        BV_FLD(GPMI_CTRL0, WORD_LENGTH, 8_BIT)              |
                                        BV_FLD(GPMI_CTRL0, LOCK_CS, DISABLED)               |
                                        BF_GPMI_CTRL0_CS(CHIP_SEL)                          |   // must correspond to NAND CS used
                                        BV_FLD(GPMI_CTRL0, ADDRESS, NAND_DATA)              |
                                        BF_GPMI_CTRL0_ADDRESS_INCREMENT(0)                  |
                                        BF_GPMI_CTRL0_XFER_COUNT(1);
    chains_erase[5].gpmi_compare    =   MASK_AND_REFERENCE_VALUE;                           // NOTE: mask and reference values are NAND
                                                                                            // SPECIFIC to evaluate the NAND status
    //  Phase 7: psense compare (time out check)

    chains_erase[6].dma_nxtcmdar    =   (uint32_t)&chains_erase[7];                         // point to the next descriptor
    chains_erase[6].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(0)                       |   // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(0)                         |   // no words sent to GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(0)                      |   // do not wait to continue
                                        BF_APBH_CHn_CMD_SEMAPHORE(0)                        |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)                   |
                                        BF_APBH_CHn_CMD_NANDLOCK(0)                         |   // relinquish nand lock
                                        BF_APBH_CHn_CMD_IRQONCMPLT(0)                       |
                                        BF_APBH_CHn_CMD_CHAIN(1)                            |   // follow chain to next command
                                        BV_FLD(APBH_CHn_CMD, COMMAND, DMA_SENSE);               // perform a sense check
    chains_erase[6].dma_bar         =   (uint32_t)&chains_erase[8];                      // if sense check fails, branch to error handler

    //  Phase 8: emit GPMI interrupt
    chains_erase[7].dma_nxtcmdar    =   0;           // not used since this is last descriptor
    chains_erase[7].dma_cmd         =   BF_APBH_CHn_CMD_XFER_COUNT(0)                   |   // no dma transfer
                                        BF_APBH_CHn_CMD_CMDWORDS(0)                     |   // no words sent to GPMI
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(0)                  |   // do not wait to continue
                                        BF_APBH_CHn_CMD_SEMAPHORE(1)                    |
                                        BF_APBH_CHn_CMD_NANDWAIT4READY(0)               |
                                        BF_APBH_CHn_CMD_NANDLOCK(0)                     |
                                        BF_APBH_CHn_CMD_IRQONCMPLT(1)                   |   // emit GPMI interrupt
                                        BF_APBH_CHn_CMD_CHAIN(0)                        |   // terminate DMA chain processing
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);         // no dma transfer

    //  ERROR Brunch;
    chains_erase[8].dma_cmd          =  BF_APBH_CHn_CMD_IRQONCMPLT(1)               |
                                        BF_APBH_CHn_CMD_WAIT4ENDCMD(0)              |
                                        BF_APBH_CHn_CMD_SEMAPHORE(1)                |
                                        BV_FLD(APBH_CHn_CMD, COMMAND, NO_DMA_XFER);

    chains_erase[8].dma_bar          =   0;
    chains_erase[8].dma_nxtcmdar     =   0;
}


void GPMI_SendCMD(uint32_t *cmd, uint32_t *par, uint16_t par_len, uint32_t *rec_buf, uint32_t readback_len)
{
    while (HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA)
        ;
    chains_cmd[1].dma_cmd &= 0x0000FFFF;
    chains_cmd[1].dma_cmd |= (1 + par_len) << 16;
    chains_cmd[1].dma_bar = (uint32_t)cmd;

    chains_cmd[1].gpmi_ctrl0 &= 0xFFFF0000;
    chains_cmd[1].gpmi_ctrl0 |= ((1 + par_len) & 0xFFFF);

    if(par_len){
        chains_cmd[1].dma_nxtcmdar  =   (uint32_t)&chains_cmd[2];

        chains_cmd[2].dma_cmd &= 0x0000FFFF;
        chains_cmd[2].dma_cmd |= (1 + readback_len) << 16;
        chains_cmd[2].gpmi_ctrl0 &= 0xFFFF0000;
        chains_cmd[2].gpmi_ctrl0 |= ((1 + readback_len) & 0xFFFF);
        chains_cmd[2].dma_bar = (uint32_t)rec_buf;

    }else{
        chains_cmd[1].dma_nxtcmdar  =   (uint32_t)&chains_cmd[3];
    }
    
    //mmu_clean_invalidated_dcache(chains_cmd, sizeof(chains_cmd));
    //mmu_drain_buffer();

    BF_WRn(APBH_CHn_NXTCMDAR, NAND_DMA_Channel, CMD_ADDR, (reg32_t)&chains_cmd);
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(NAND_DMA_Channel, 1);

    while (HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA)
            ;
    while(!BF_RD(APBH_CTRL1, CH4_CMDCMPLT_IRQ))
        ;
    
    BF_CLR(APBH_CTRL1, CH4_CMDCMPLT_IRQ);
    BF_CLR(APBH_CTRL1, CH4_AHB_ERROR_IRQ);
}

void bsp_nand_reset()
{
    FlashSendCommandBuffer[0] = 0xFF;
    //mmu_clean_invalidated_dcache(FlashSendCommandBuffer, sizeof(FlashSendCommandBuffer));
    GPMI_SendCMD(FlashSendCommandBuffer, 0, 0, 0, 0);
    bsp_delayus(20);
}

uint32_t* bsp_nand_getid()
{
    FlashSendCommandBuffer[0] = 0x90;
    FlashSendParaBuffer[0] = 0;
    GPMI_SendCMD(FlashSendCommandBuffer, FlashSendParaBuffer, 1, FlashRecCommandBuffer, 6);
    INFO("Flash ID:\r\n");
    for(int i = 0; i < 6;i++){
        INFO("%02x ", ((uint8_t *)FlashRecCommandBuffer)[i] );
    }
    INFO("\n");
    char DIDesc4Rd = ((uint8_t *)FlashRecCommandBuffer)[3];
    uint32_t PageSize_B = (1 << ( DIDesc4Rd & 0x3 )) * 1024 ;
    uint32_t SpareSizePerPage_B = PageSize_B / 512 * ((1 << ((DIDesc4Rd >> 2) & 1)) * 8);
    uint32_t BlockSize_KB = (1 << ((DIDesc4Rd >> 4) & 0x3)) * 64;
    uint32_t PagesPerBlock = BlockSize_KB * 1024 / PageSize_B;

    INFO("PageSize:%lu B\n", PageSize_B);
    INFO("SpareSizePerPage:%lu B\n", SpareSizePerPage_B);
    INFO("BlockSize:%lu KB\n", BlockSize_KB);
    INFO("PagesPerBlock:%lu\n", PagesPerBlock);
    page_sz = PageSize_B;
    return FlashRecCommandBuffer;
}

int static nand_read(uint32_t page, uint32_t *ECC)
{
    int ret = 0;
    volatile uint8_t *cmdBuf = (uint8_t *)FlashSendCommandBuffer;
    while ((HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA))
        ;

    cmdBuf[0] = NAND_CMD_READ0;
    cmdBuf[1] = 0 & 0xFF;
    cmdBuf[2] = (0 >> 8) & 0xFF;
    cmdBuf[3] = page & 0xFF;
    cmdBuf[4] = (page >> 8) & 0xFF;
    cmdBuf[5] = NAND_CMD_READSTART;
    chains_read[4].gpmi_data_ptr    =   (uint32_t)GPMI_DataBuffer;
    chains_read[4].gpmi_aux_ptr     =   (uint32_t)GPMI_AuxiliaryBuffer;
    
    //mmu_drain_buffer();
    //mmu_clean_invalidated_dcache(chains_cmd, sizeof(chains_cmd));

    BF_WRn(APBH_CHn_NXTCMDAR, NAND_DMA_Channel, CMD_ADDR, (reg32_t)&chains_read[0]); 
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(NAND_DMA_Channel, 1);

    while ((HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA))
        ;
    while(!BF_RD(APBH_CTRL1, CH4_CMDCMPLT_IRQ))
        ;
    while((!BF_RD(ECC8_CTRL, COMPLETE_IRQ)) && (!BF_RD(ECC8_CTRL, BM_ERROR_IRQ)))
        ;

    if(BF_RD(GPMI_CTRL1, TIMEOUT_IRQ))
        INFO("GPMI_TIMEOUT_IRQ\n");
    if(BF_RD(GPMI_CTRL1, DEV_IRQ))
        INFO("GPMI_DEV_IRQ\n");

    if(BF_RDn(APBH_CHn_CURCMDAR, NAND_DMA_Channel, CMD_ADDR) == (uint32_t)&chains_read[8]){
        ret = -1;
        INFO("GPMI_OPA_READ psense compare ERROR\n");
    }

    if(BF_RD(ECC8_CTRL, COMPLETE_IRQ))
    {
        if(ECC)
        {
            *ECC = BF_RD(ECC8_STATUS1, STATUS_PAYLOAD0)         |
                (BF_RD(ECC8_STATUS1, STATUS_PAYLOAD1) << 8)     |
                (BF_RD(ECC8_STATUS1, STATUS_PAYLOAD2) << 16)    |
                (BF_RD(ECC8_STATUS1, STATUS_PAYLOAD3) << 24)    ;
        }
    }

    if(BF_RD(ECC8_CTRL, BM_ERROR_IRQ))
    {
        INFO("NAND ECC ERROR! page:%ld\r\n",page);
        ret = -1;
    }
    
    BF_CLR(GPMI_CTRL1, DEV_IRQ);
    BF_CLR(GPMI_CTRL1, TIMEOUT_IRQ);
    BF_CLR(ECC8_CTRL, COMPLETE_IRQ);
    BF_CLR(ECC8_CTRL, BM_ERROR_IRQ);
    BF_CLR(APBH_CTRL1, CH4_CMDCMPLT_IRQ);
    BF_CLR(APBH_CTRL1, CH4_AHB_ERROR_IRQ);

    while(HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.AHB_BYTES);
    while(HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.APB_BYTES);

    bsp_delayus(defaultTiming.tREAD_us);
    return ret;
}

int bsp_nand_read_page(uint32_t page, uint8_t *buffer, uint32_t *ECC)
{

    int ret = nand_read(page, ECC);
    //mmu_invalidate_dcache(GPMI_DataBuffer,page_sz);
    //mmu_invalidate_dcache(GPMI_AuxiliaryBuffer,19);
    if(buffer)
    {
        memcpy(buffer, (void *)GPMI_DataBuffer,page_sz);
        memcpy(&buffer[page_sz], (void *)GPMI_AuxiliaryBuffer, 19);
    }
    return ret;
}

int bsp_nand_read_page_no_meta(uint32_t page, uint8_t *buffer, uint32_t *ECC)
{

    int ret = nand_read(page, ECC);
    //mmu_invalidate_dcache(GPMI_DataBuffer,page_sz);
    //mmu_invalidate_dcache(GPMI_AuxiliaryBuffer,19);
    if(buffer)
    {
        memcpy(buffer, (void *)GPMI_DataBuffer,page_sz);
    }
    return ret;
}

void bsp_nand_erase_block(uint32_t block)
{
    volatile uint8_t *cmdBuf = (uint8_t *)FlashSendCommandBuffer;
    while ((HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA))
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
    while(!BF_RD(APBH_CTRL1, CH4_CMDCMPLT_IRQ))
        ;

    if(BF_RDn(APBH_CHn_CURCMDAR, NAND_DMA_Channel, CMD_ADDR) == (uint32_t)&chains_erase[8]){
        INFO("GPMI_OPA_READ psense compare ERROR\n");
    }

    
    while(HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.AHB_BYTES);
    while(HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.APB_BYTES);

    BF_CLR(ECC8_CTRL, COMPLETE_IRQ);
    BF_CLR(ECC8_CTRL, BM_ERROR_IRQ);
    BF_CLR(APBH_CTRL1, CH4_CMDCMPLT_IRQ);
    BF_CLR(APBH_CTRL1, CH4_AHB_ERROR_IRQ);
    bsp_delayus(defaultTiming.tBERS_us);

}

static void nand_write(uint32_t page)
{
    while(HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.AHB_BYTES);
    while(HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.APB_BYTES);
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

    chains_write[1].dma_bar     =   (uint32_t)GPMI_DataBuffer;
    chains_write[2].dma_bar     =   (uint32_t)GPMI_AuxiliaryBuffer;

    BF_WRn(APBH_CHn_NXTCMDAR, NAND_DMA_Channel, CMD_ADDR, (reg32_t)&chains_write[0]); 
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(NAND_DMA_Channel, 1); 

    while ((HW_APBH_CHn_SEMA(NAND_DMA_Channel).B.INCREMENT_SEMA))
        ;
    while(!BF_RD(APBH_CTRL1, CH4_CMDCMPLT_IRQ))
        ;

    if(BF_RD(GPMI_CTRL1, TIMEOUT_IRQ))
        INFO("GPMI_TIMEOUT_IRQ\n");
    if(BF_RD(GPMI_CTRL1, DEV_IRQ))
        INFO("GPMI_DEV_IRQ\n");

    if(BF_RDn(APBH_CHn_CURCMDAR, NAND_DMA_Channel, CMD_ADDR) == (uint32_t)&chains_erase[8]){
        INFO("GPMI_OPA_WRITE psense compare ERROR\n");
    }
    

    BF_CLR(GPMI_CTRL1, DEV_IRQ);
    BF_CLR(GPMI_CTRL1, TIMEOUT_IRQ);
    BF_CLR(ECC8_CTRL, COMPLETE_IRQ);
    BF_CLR(ECC8_CTRL, BM_ERROR_IRQ);
    BF_CLR(APBH_CTRL1, CH4_CMDCMPLT_IRQ);
    BF_CLR(APBH_CTRL1, CH4_AHB_ERROR_IRQ);

    while(HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.AHB_BYTES);
    while(HW_APBH_CHn_DEBUG2(NAND_DMA_Channel).B.APB_BYTES);


    bsp_delayus(defaultTiming.tPROG_us);
}

void bsp_nand_write_page(uint32_t page, uint8_t *dat)
{
    memcpy((void *)GPMI_DataBuffer, dat, page_sz);
    memcpy((void *)GPMI_AuxiliaryBuffer,&dat[page_sz], 19);
    nand_write(page);
}

void bsp_nand_write_page_nometa(uint32_t page, uint8_t *dat)
{
    memcpy((void *)GPMI_DataBuffer, dat, page_sz);
    memset((void *)GPMI_AuxiliaryBuffer, 0xFF, 19);
    nand_write(page);
}

bool bsp_nand_check_is_bad_block(uint32_t block)
{
    memset((void*)GPMI_AuxiliaryBuffer,0xff,sizeof(GPMI_AuxiliaryBuffer));
    bsp_nand_read_page(64 * block, NULL, NULL);
    return !GPMI_AuxiliaryBuffer[0];
    //return false;
}

void bsp_nand_init()
{
    interface_init();
    GPMI_DMAChainsInit();
    bsp_nand_reset();
    bsp_nand_getid();


}