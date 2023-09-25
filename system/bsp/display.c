#include "board.h" 
#include "utils.h"
#include <string.h>

//#include "FreeRTOS.h"
//#include "task.h"
//#include "semphr.h"

//static SemaphoreHandle_t lock;

extern unsigned char Ascii1608[];

#define DISPLAY_INVERSE (1)

#define SCREEN_START_Y (8) // 0 - 126
#define SCREEN_END_Y (134)

#define SCREEN_START_X (0)
#define SCREEN_END_X (255 / 3) // 0 - 255

#define SCREEN_WIDTH (256)
#define SCREEN_HEIGHT (127)

DMA_MEM static uint8_t line_Buffer[SCREEN_WIDTH + 4] __aligned(4);
static uint32_t cur_line = 0;
static bool linebuf_dirty = false;

typedef struct LCDIF_DMADesc
{
    volatile struct LCDIF_DMADesc *pNext;
    union
    {
        struct
        {
            union
            {
                struct
                {
                    uint8_t DMA_Command : 2;
                    uint8_t DMA_Chain : 1;
                    uint8_t DMA_IRQOnCompletion : 1;
                    uint8_t DMA_NANDLock : 1;
                    uint8_t DMA_NANDWaitForReady : 1;
                    uint8_t DMA_Semaphore : 1;
                    uint8_t DMA_WaitForEndCommand : 1;
                };
                uint8_t Bits;
            };
            uint8_t Reserved : 4;
            uint8_t DMA_PIOWords : 4;
            uint16_t DMA_XferBytes : 16;
        };
        uint32_t DMA_CommandBits;
    };
    uint32_t pDMABuffer;
    hw_lcdif_ctrl_t PioWord;

} LCDIF_DMADesc;

typedef struct LCDIF_Timing_t
{
    unsigned char DataSetup_ns;
    unsigned char DataHold_ns;
    unsigned char CmdSetup_ns;
    unsigned char CmdHold_ns;
    uint32_t minOpaTime_us;
    uint32_t minReadBackTime_us;
} LCDIF_Timing_t;

static struct LCDIF_Timing_t defaultTiming =
    {
        .DataSetup_ns = 50 + 10,
        .DataHold_ns = 0 + 0,
        .CmdSetup_ns = 15 + 10,
        .CmdHold_ns = 15 + 10,
        .minOpaTime_us = 10,
        .minReadBackTime_us = 30};

static uint64_t LCDIFFreq;

//DMA_MEM volatile static LCDIF_DMADesc chains_wr;

DMA_MEM volatile static LCDIF_DMADesc chains_wr_rd_line[6+2];

DMA_MEM volatile __aligned(4) const static uint8_t CMD_2A = 0x2A;
DMA_MEM volatile __aligned(4) const static uint8_t CMD_2B = 0x2B;
DMA_MEM volatile __aligned(4) const static uint8_t CMD_2C = 0x2C;
DMA_MEM volatile __aligned(4) const static uint8_t CMD_2E = 0x2E;
DMA_MEM volatile static uint32_t DAT_2A;
DMA_MEM volatile static uint32_t DAT_2B;
DMA_MEM volatile static uint32_t CMD;
DMA_MEM volatile static uint32_t DAT;



static bool LCDIF_checkSendFinish()
{
    return BF_RD(LCDIF_STAT, TXFIFO_EMPTY);
}

static bool LCDIF_checkDMAFin()
{
    // return true;//((HW_APBH_CHn_DEBUG2(0).B.AHB_BYTES) && (HW_APBH_CHn_DEBUG2(0).B.APB_BYTES));
    return ((HW_APBH_CHn_SEMA(0).B.INCREMENT_SEMA) == 0);
}

static void LCDIF_DMAChainsInit()
{
//    memset((void *)&chains_wr, 0, sizeof(chains_wr));
    memset((void *)&chains_wr_rd_line, 0, sizeof(chains_wr_rd_line));
/*
    chains_wr.pNext = NULL; 
    chains_wr.DMA_Semaphore = 1;
    chains_wr.DMA_Command = BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);
    chains_wr.DMA_Chain = 0;
    chains_wr.DMA_IRQOnCompletion = 1; 
    chains_wr.DMA_PIOWords = 1;
    chains_wr.pDMABuffer = 00000000;
    chains_wr.DMA_XferBytes = 0000;

    chains_wr.PioWord.B.COUNT = 0000;
    chains_wr.PioWord.B.WORD_LENGTH = 1; // 8bit mode
    chains_wr.PioWord.B.DATA_SELECT = 0; // 0:command mode   1:data mode
    chains_wr.PioWord.B.RUN = 1;
    chains_wr.PioWord.B.BYPASS_COUNT = 0;
    chains_wr.PioWord.B.READ_WRITEB = 0; // 0:write mode  1:read mode
*/
    chains_wr_rd_line[0].pNext = &chains_wr_rd_line[1];
    chains_wr_rd_line[0].DMA_Semaphore = 0;
    chains_wr_rd_line[0].DMA_Command =  BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);
    chains_wr_rd_line[0].DMA_Chain = 1;
    chains_wr_rd_line[0].DMA_IRQOnCompletion = 0;
    chains_wr_rd_line[0].DMA_PIOWords = 1;
    chains_wr_rd_line[0].pDMABuffer = (uint32_t)&CMD_2A;
    chains_wr_rd_line[0].DMA_WaitForEndCommand = 1;
    chains_wr_rd_line[0].DMA_XferBytes = 1;
    chains_wr_rd_line[0].PioWord.B.COUNT = 1;
    chains_wr_rd_line[0].PioWord.B.WORD_LENGTH = 1; // 8bit mode
    chains_wr_rd_line[0].PioWord.B.DATA_SELECT = 0; // 0:command mode   1:data mode
    chains_wr_rd_line[0].PioWord.B.RUN = 1;
    chains_wr_rd_line[0].PioWord.B.BYPASS_COUNT = 0;
    chains_wr_rd_line[0].PioWord.B.READ_WRITEB = 0; // 0:write mode  1:read mode


    chains_wr_rd_line[1].pNext = &chains_wr_rd_line[2];
    chains_wr_rd_line[1].DMA_Semaphore = 0;
    chains_wr_rd_line[1].DMA_Command =  BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);
    chains_wr_rd_line[1].DMA_Chain = 1;
    chains_wr_rd_line[1].DMA_IRQOnCompletion = 0;
    chains_wr_rd_line[1].DMA_PIOWords = 1;
    chains_wr_rd_line[1].pDMABuffer = (uint32_t)&DAT_2A;
    chains_wr_rd_line[1].DMA_WaitForEndCommand = 1;
    chains_wr_rd_line[1].DMA_XferBytes = 4;
    chains_wr_rd_line[1].PioWord.B.COUNT = 4;
    chains_wr_rd_line[1].PioWord.B.WORD_LENGTH = 1; // 8bit mode
    chains_wr_rd_line[1].PioWord.B.DATA_SELECT = 1; // 0:command mode   1:data mode
    chains_wr_rd_line[1].PioWord.B.RUN = 1;
    chains_wr_rd_line[1].PioWord.B.BYPASS_COUNT = 0;
    chains_wr_rd_line[1].PioWord.B.READ_WRITEB = 0; // 0:write mode  1:read mode

    chains_wr_rd_line[2].pNext = &chains_wr_rd_line[3];
    chains_wr_rd_line[2].DMA_Semaphore = 0;
    chains_wr_rd_line[2].DMA_Command =  BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);
    chains_wr_rd_line[2].DMA_Chain = 1;
    chains_wr_rd_line[2].DMA_IRQOnCompletion = 0;
    chains_wr_rd_line[2].DMA_PIOWords = 1;
    chains_wr_rd_line[2].pDMABuffer = (uint32_t)&CMD_2B;
    chains_wr_rd_line[2].DMA_WaitForEndCommand = 1;
    chains_wr_rd_line[2].DMA_XferBytes = 1;
    chains_wr_rd_line[2].PioWord.B.COUNT = 1;
    chains_wr_rd_line[2].PioWord.B.WORD_LENGTH = 1; // 8bit mode
    chains_wr_rd_line[2].PioWord.B.DATA_SELECT = 0; // 0:command mode   1:data mode
    chains_wr_rd_line[2].PioWord.B.RUN = 1;
    chains_wr_rd_line[2].PioWord.B.BYPASS_COUNT = 0;
    chains_wr_rd_line[2].PioWord.B.READ_WRITEB = 0; // 0:write mode  1:read mode

    chains_wr_rd_line[3].pNext = &chains_wr_rd_line[4];
    chains_wr_rd_line[3].DMA_Semaphore = 0;
    chains_wr_rd_line[3].DMA_Command =  BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);
    chains_wr_rd_line[3].DMA_Chain = 1;
    chains_wr_rd_line[3].DMA_IRQOnCompletion = 0;
    chains_wr_rd_line[3].DMA_PIOWords = 1;
    chains_wr_rd_line[3].pDMABuffer = (uint32_t)&DAT_2B;
    chains_wr_rd_line[3].DMA_WaitForEndCommand = 1;
    chains_wr_rd_line[3].DMA_XferBytes = 4;
    chains_wr_rd_line[3].PioWord.B.COUNT = 4;
    chains_wr_rd_line[3].PioWord.B.WORD_LENGTH = 1; // 8bit mode
    chains_wr_rd_line[3].PioWord.B.DATA_SELECT = 1; // 0:command mode   1:data mode
    chains_wr_rd_line[3].PioWord.B.RUN = 1;
    chains_wr_rd_line[3].PioWord.B.BYPASS_COUNT = 0;
    chains_wr_rd_line[3].PioWord.B.READ_WRITEB = 0; // 0:write mode  1:read mode

//  ==============================  Write ============================================

    chains_wr_rd_line[4].pNext = &chains_wr_rd_line[5];
    chains_wr_rd_line[4].DMA_Semaphore = 0;
    chains_wr_rd_line[4].DMA_Command =  BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);
    chains_wr_rd_line[4].DMA_Chain = 1;
    chains_wr_rd_line[4].DMA_IRQOnCompletion = 0;
    chains_wr_rd_line[4].DMA_PIOWords = 1;
    chains_wr_rd_line[4].pDMABuffer = (uint32_t)&CMD_2C;
    chains_wr_rd_line[4].DMA_WaitForEndCommand = 1;
    chains_wr_rd_line[4].DMA_XferBytes = 1;
    chains_wr_rd_line[4].PioWord.B.COUNT = 1;
    chains_wr_rd_line[4].PioWord.B.WORD_LENGTH = 1; // 8bit mode
    chains_wr_rd_line[4].PioWord.B.DATA_SELECT = 0; // 0:command mode   1:data mode
    chains_wr_rd_line[4].PioWord.B.RUN = 1;
    chains_wr_rd_line[4].PioWord.B.BYPASS_COUNT = 0;
    chains_wr_rd_line[4].PioWord.B.READ_WRITEB = 0; // 0:write mode  1:read mode

    chains_wr_rd_line[5].pNext = NULL;
    chains_wr_rd_line[5].DMA_Semaphore = 1;
    chains_wr_rd_line[5].DMA_Command =  BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);
    chains_wr_rd_line[5].DMA_Chain = 0;
    chains_wr_rd_line[5].DMA_IRQOnCompletion = 1;
    chains_wr_rd_line[5].DMA_PIOWords = 1;
    chains_wr_rd_line[5].pDMABuffer = (uint32_t)&line_Buffer[0];
    chains_wr_rd_line[5].DMA_XferBytes = 256+2;
    chains_wr_rd_line[5].PioWord.B.COUNT =  256+2;
    chains_wr_rd_line[5].PioWord.B.WORD_LENGTH = 1; // 8bit mode
    chains_wr_rd_line[5].PioWord.B.DATA_SELECT = 1; // 0:command mode   1:data mode
    chains_wr_rd_line[5].PioWord.B.RUN = 1;
    chains_wr_rd_line[5].PioWord.B.BYPASS_COUNT = 0;
    chains_wr_rd_line[5].PioWord.B.READ_WRITEB = 0; // 0:write mode  1:read mode

//  ==============================  Read ============================================

    chains_wr_rd_line[6].pNext = &chains_wr_rd_line[7];
    chains_wr_rd_line[6].DMA_Semaphore = 0;
    chains_wr_rd_line[6].DMA_Command =  BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);
    chains_wr_rd_line[6].DMA_Chain = 1;
    chains_wr_rd_line[6].DMA_IRQOnCompletion = 0;
    chains_wr_rd_line[6].DMA_PIOWords = 1;
    chains_wr_rd_line[6].pDMABuffer = (uint32_t)&CMD_2E;
    chains_wr_rd_line[6].DMA_WaitForEndCommand = 1;
    chains_wr_rd_line[6].DMA_XferBytes = 1;
    chains_wr_rd_line[6].PioWord.B.COUNT = 1;
    chains_wr_rd_line[6].PioWord.B.WORD_LENGTH = 1; // 8bit mode
    chains_wr_rd_line[6].PioWord.B.DATA_SELECT = 0; // 0:command mode   1:data mode
    chains_wr_rd_line[6].PioWord.B.RUN = 1;
    chains_wr_rd_line[6].PioWord.B.BYPASS_COUNT = 0;
    chains_wr_rd_line[6].PioWord.B.READ_WRITEB = 0; // 0:write mode  1:read mode

    chains_wr_rd_line[7].pNext = NULL;
    chains_wr_rd_line[7].DMA_Semaphore = 1;
    chains_wr_rd_line[7].DMA_Command =  BV_FLD(APBH_CHn_CMD, COMMAND, DMA_WRITE);
    chains_wr_rd_line[7].DMA_Chain = 0;
    chains_wr_rd_line[7].DMA_IRQOnCompletion = 1;
    chains_wr_rd_line[7].DMA_PIOWords = 1;
    chains_wr_rd_line[7].pDMABuffer = (uint32_t)&line_Buffer[0];
    chains_wr_rd_line[7].DMA_XferBytes = 256+2;
    chains_wr_rd_line[7].PioWord.B.COUNT =  256+2;
    chains_wr_rd_line[7].PioWord.B.WORD_LENGTH = 1; // 8bit mode
    chains_wr_rd_line[7].PioWord.B.DATA_SELECT = 1; // 0:command mode   1:data mode
    chains_wr_rd_line[7].PioWord.B.RUN = 1;
    chains_wr_rd_line[7].PioWord.B.BYPASS_COUNT = 0;
    chains_wr_rd_line[7].PioWord.B.READ_WRITEB = 1; // 0:write mode  1:read mode



}



void lcd_write_line(uint32_t line)
{
    //xSemaphoreTake(lock, portMAX_DELAY);
    
    while ((HW_APBH_CHn_SEMA(0).B.INCREMENT_SEMA))
        ;
    while (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
        ;

    DAT_2A = __builtin_bswap16(0/3) | (__builtin_bswap16(255/3) << 16);
    DAT_2B = __builtin_bswap16(line) | ((__builtin_bswap16(line) << 16));

    chains_wr_rd_line[3].pNext = &chains_wr_rd_line[4];
    BF_WRn(APBH_CHn_NXTCMDAR, 0, CMD_ADDR, (reg32_t)&chains_wr_rd_line[0]);
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(0, 1);

    while ((HW_APBH_CHn_SEMA(0).B.INCREMENT_SEMA))
        ;
    while (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
        ;

    if (BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ) || BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ))
    {
        INFO("LCDIF ERR IRQ, Overflow:%d, Underflow:%d\n",
             BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ),
             BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ));
        BF_CLR(LCDIF_CTRL1, OVERFLOW_IRQ);
        BF_CLR(LCDIF_CTRL1, UNDERFLOW_IRQ);
    }

    bsp_delayus(50);
    
    //xSemaphoreGive(lock);
}


/*
void lcd_wr_cmd(uint32_t cmd, uint32_t len)
{
    while ((HW_APBH_CHn_SEMA(0).B.INCREMENT_SEMA))
        ;
    while (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
        ;
    while (lock)
        ;
    lock++;

    CMD = cmd;
    
    chains_wr.DMA_Command = BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);
    chains_wr.pDMABuffer = (uint32_t)&CMD;
    chains_wr.DMA_XferBytes = len;
    chains_wr.PioWord.B.COUNT = len;
    chains_wr.PioWord.B.WORD_LENGTH = 1; // 8bit mode
    chains_wr.PioWord.B.DATA_SELECT = 0; // 0:command mode   1:data mode
    chains_wr.PioWord.B.RUN = 1;
    chains_wr.PioWord.B.BYPASS_COUNT = 0;
    chains_wr.PioWord.B.READ_WRITEB = 0; // 0:write mode  1:read mode

    BF_WRn(APBH_CHn_NXTCMDAR, 0, CMD_ADDR, (reg32_t)&chains_wr);
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(0, 1);

    while ((HW_APBH_CHn_SEMA(0).B.INCREMENT_SEMA))
        ;
    while (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
        ;

    if (BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ) || BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ))
    {
        INFO("LCDIF ERR IRQ, Overflow:%d, Underflow:%d\n",
             BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ),
             BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ));
        BF_CLR(LCDIF_CTRL1, OVERFLOW_IRQ);
        BF_CLR(LCDIF_CTRL1, UNDERFLOW_IRQ);
    }

    bsp_delayus(5);
    lock--;
}

void lcd_wr_dat(uint32_t dat, uint32_t len)
{
    while ((HW_APBH_CHn_SEMA(0).B.INCREMENT_SEMA))
        ;
    while (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
        ;
    while (lock)
        ;
    lock++;

    DAT = dat;
    
    chains_wr.DMA_Command = BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);
    chains_wr.pDMABuffer = (uint32_t)&DAT;
    chains_wr.DMA_XferBytes = len;
    chains_wr.PioWord.B.COUNT = len;
    chains_wr.PioWord.B.WORD_LENGTH = 1; // 8bit mode
    chains_wr.PioWord.B.DATA_SELECT = 1; // 0:command mode   1:data mode
    chains_wr.PioWord.B.RUN = 1;
    chains_wr.PioWord.B.BYPASS_COUNT = 0;
    chains_wr.PioWord.B.READ_WRITEB = 0; // 0:write mode  1:read mode

    BF_WRn(APBH_CHn_NXTCMDAR, 0, CMD_ADDR, (reg32_t)&chains_wr);
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(0, 1);

    while ((HW_APBH_CHn_SEMA(0).B.INCREMENT_SEMA))
        ;
    while (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
        ;

    if (BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ) || BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ))
    {
        INFO("LCDIF ERR IRQ, Overflow:%d, Underflow:%d\n",
             BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ),
             BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ));
        BF_CLR(LCDIF_CTRL1, OVERFLOW_IRQ);
        BF_CLR(LCDIF_CTRL1, UNDERFLOW_IRQ);
    }

    bsp_delayus(5);
    lock--;
}

void lcd_rd_dat(uint32_t len, uint32_t PA)
{
    while ((HW_APBH_CHn_SEMA(0).B.INCREMENT_SEMA))
        ;
    while (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
        ;
    while (lock)
        ;
    lock++;

    chains_wr.DMA_Command = BV_FLD(APBH_CHn_CMD, COMMAND, DMA_WRITE);
    chains_wr.pDMABuffer = PA;
    chains_wr.DMA_XferBytes = len;
    chains_wr.PioWord.B.COUNT = len;
    chains_wr.PioWord.B.WORD_LENGTH = 1; // 8bit mode
    chains_wr.PioWord.B.DATA_SELECT = 1; // 0:command mode   1:data mode
    chains_wr.PioWord.B.RUN = 1;
    chains_wr.PioWord.B.BYPASS_COUNT = 0;
    chains_wr.PioWord.B.READ_WRITEB = 1; // 0:write mode  1:read mode

    BF_WRn(APBH_CHn_NXTCMDAR, 0, CMD_ADDR, (reg32_t)&chains_wr);
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(0, 1);

    while ((HW_APBH_CHn_SEMA(0).B.INCREMENT_SEMA))
        ;
    while (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
        ;

    if (BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ) || BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ))
    {
        INFO("LCDIF ERR IRQ, Overflow:%d, Underflow:%d\n",
             BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ),
             BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ));
        BF_CLR(LCDIF_CTRL1, OVERFLOW_IRQ);
        BF_CLR(LCDIF_CTRL1, UNDERFLOW_IRQ);
    }

    bsp_delayus(5);
    lock--;
}
*/
void lcd_read_line(uint32_t line)
{
    
    //xSemaphoreTake(lock, portMAX_DELAY);

    while ((HW_APBH_CHn_SEMA(0).B.INCREMENT_SEMA))
        ;
    while (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
        ;
    
    DAT_2A = __builtin_bswap16(0/3) | (__builtin_bswap16(255/3) << 16);
    DAT_2B = __builtin_bswap16(line) | ((__builtin_bswap16(line) << 16));

    chains_wr_rd_line[3].pNext = &chains_wr_rd_line[6];
    BF_WRn(APBH_CHn_NXTCMDAR, 0, CMD_ADDR, (reg32_t)&chains_wr_rd_line[0]);
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(0, 1);

    while ((HW_APBH_CHn_SEMA(0).B.INCREMENT_SEMA))
        ;
    while (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
        ;

    if (BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ) || BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ))
    {
        INFO("LCDIF ERR IRQ, Overflow:%d, Underflow:%d\n",
             BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ),
             BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ));
        BF_CLR(LCDIF_CTRL1, OVERFLOW_IRQ);
        BF_CLR(LCDIF_CTRL1, UNDERFLOW_IRQ);
    }

    bsp_delayus(50);
    
    //xSemaphoreGive(lock);
}

void lcd_test2()
{
    memset(line_Buffer, 0, sizeof(line_Buffer));
    for(int i = 0; i<sizeof(line_Buffer);i++)
    {
        line_Buffer[i] = i & 0xFF;
    }
    for(int i = 0 + 8; i < 62 + 8; i++){
        lcd_write_line(i);
    }


    for(int i = 0; i<sizeof(line_Buffer);i++)
    {
        line_Buffer[i] = i & 0xF8;
    }
    for(int i = 64 + 8; i < 126 + 8; i++){
        lcd_write_line(i);
    }

}


void lcd_test()
{
    
    static int s = 0;
    if(s == 0)
    {
        memset(line_Buffer, 0xFF, sizeof(line_Buffer));
        s = 1;
        for(int i = 8; i < 134; i++){
            lcd_write_line(i);
        }
        return;
    }

    for(int i = 8; i < 134; i++){
        
        lcd_read_line(i);
        if(i == 45){
            INFO("RB: ");
            for(int j = 0; j < 6; j++)
            {
                INFO("%02X ", line_Buffer[i]);
            }
            INFO("\r\n");
        }
    for(int i=0; i < sizeof(line_Buffer); i++)
    {
        line_Buffer[i] -= 0x8;
    }
        lcd_write_line(i);
    }
/*
    for(int i = 8; i < 134; i++){
        lcd_write_line(i);
    }
    memset(line_Buffer, 0, sizeof(line_Buffer));

    lcd_read_line(12);
    
    INFO("RB: ");
    for(int i = 0; i < 6; i++)
    {
        INFO("%02X ", line_Buffer[i]);
    }
    INFO("\r\n");
    for(int i=0; i < sizeof(line_Buffer); i++)
    {
        line_Buffer[i] -= 0x10;
    }*/
}

void bsp_diaplay_clean(uint32_t c)
{
    uint16_t start_x = SCREEN_START_X;
    uint16_t start_y = 0;
    uint16_t end_x = SCREEN_END_X;
    uint16_t end_y = SCREEN_END_Y;

    memset(line_Buffer, c, sizeof(line_Buffer));

    for(int i = 8; i < 134; i++){
        lcd_write_line(i);
    }
    linebuf_dirty = false;
}
 
void bsp_display_flush()
{
    if(linebuf_dirty)
    {
        lcd_write_line(cur_line);
        linebuf_dirty = false;
    }   
}

void bsp_display_set_point(uint32_t x, uint32_t y, uint32_t c)
{
    if ((x >= SCREEN_WIDTH) || (y >= SCREEN_HEIGHT))
        return;
    uint32_t line = y + 8;
    if(cur_line != line)
    {
        if(cur_line)
        {
            lcd_write_line(cur_line);
        }
        lcd_read_line(line);
        cur_line = line;
    }
    line_Buffer[x] = c;
    linebuf_dirty = true;
}

uint32_t bsp_display_get_point(uint32_t x, uint32_t y)
{
    if ((x >= SCREEN_WIDTH) || (y >= SCREEN_HEIGHT))
        return 0;
    uint32_t line = y + 8;
    if(cur_line != line)
    {
        bsp_display_flush();
        lcd_read_line(line);
        cur_line = line;
    }
    return line_Buffer[x];
}

void bsp_diaplay_put_hline(uint32_t y, void *dat)
{
    if ((y >= SCREEN_HEIGHT))
        return;
    
    y += 8;
    
    bsp_display_flush();
    memcpy(line_Buffer, dat, 256);
    cur_line = y;
    lcd_write_line(cur_line);
}

void bsp_display_put_buffer(uint8_t *dat,uint32_t width,uint32_t height,Coords_t *coords)
{
    int coordsX=0,coordsY=0;
    if(coords!=NULL){
        coordsX=coords->x;
        coordsY=coords->y;
    }

    for(int i=0;i<height;i++)
    {
        if(coordsY+i<0)
            i=-coordsY;
        if(coordsY+i>SCREEN_HEIGHT)
            return;

        for(int j=0;j<width;j++){
            if(coordsX+j<0)
                j=-coordsX;
            if(coordsX+j>=SCREEN_WIDTH)
                continue;
            
            bsp_display_set_point(coordsX+j,coordsY+i,*(dat+j+width*i));
        }
    }
}

void bsp_display_putk_string(uint32_t x, uint32_t y, char *s, uint32_t fg, uint32_t bg)
{
    uint32_t str_len = strlen(s);
    uint8_t *font = Ascii1608;
    uint32_t fontWidth = 8;
    int fontSize = 16;
    uint32_t x0 = 0;
    for (int n = 0; n < str_len; n++)
    {
        char c = s[n];
        if((c>'~')||(c<' '))continue;
        uint32_t ch = c - ' ';
        uint8_t *p = &font[fontSize * ch];
        for (int y_ = 0; y_ < fontSize; y_++)
        {
            for (int x_ = x0; x_ < x0 + 8; x_++)
            {
                bsp_display_set_point(x + x_,y_ + y, ((p[y_] >> (7 - (x_ - x0))) & 1 ? fg : bg));
            }
        }
        x0 += fontWidth;
    }

}

void bsp_display_init()
{
    //lock = xSemaphoreCreateMutex();
    //xSemaphoreGive(lock);

    LCDIF_DMAChainsInit();
    bsp_diaplay_clean(0xFF);
}