#include "board.h"
#include "loader.h"
#include "utils.h"
#include <string.h>

extern unsigned char Ascii1608[];

#define DISPLAY_INVERSE (1)

#define SCREEN_START_Y (8) // 0 - 126
#define SCREEN_END_Y (134)

#define SCREEN_START_X (0)
#define SCREEN_END_X (255 / 3) // 0 - 255

#define SCREEN_WIDTH (256)
#define SCREEN_HEIGHT (127)

static uint32_t display_buffer[256 * 127 / 8 / 4] __aligned(4);
DMA_MEM static uint8_t line_Buffer[SCREEN_WIDTH + 4] __aligned(4);

typedef struct LCDIF_DMADesc
{
    struct LCDIF_DMADesc *pNext;
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
    uint32_t DataSetup_ns;
    uint32_t DataHold_ns;
    uint32_t CmdSetup_ns;
    uint32_t CmdHold_ns;
    uint32_t minOpaTime_us;
    uint32_t minReadBackTime_us;
} LCDIF_Timing_t;

/*
        .DataSetup_ns = 50 + 10,
        .DataHold_ns = 0 + 0,
        .CmdSetup_ns = 15 + 10,
        .CmdHold_ns = 15 + 10,
        .minOpaTime_us = 10,
        .minReadBackTime_us = 30};

LCDIFFreq:240000000
LCDIF  DATA_SETUP:23
LCDIF  DATA_HOLD:18
LCDIF  CMD_SETUP:5
LCDIF  CMD_HOLD:5     

        .DataSetup_ns = 60 ,
        .DataHold_ns =  60,
        .CmdSetup_ns = 15 ,
        .CmdHold_ns = 15 ,
        .minOpaTime_us = 15,
        .minReadBackTime_us = 30 //32us per line



*/
static struct LCDIF_Timing_t defaultTiming =
    {
        .DataSetup_ns = 65 ,
        .DataHold_ns =  65,
        .CmdSetup_ns = 20 ,
        .CmdHold_ns = 20 ,
        .minOpaTime_us = 15,
        .minReadBackTime_us = 30}; //40us per line

static uint64_t LCDIFFreq;

DMA_MEM volatile static LCDIF_DMADesc chains_wr;

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
    memset((void *)&chains_wr, 0, sizeof(chains_wr));

    chains_wr.pNext = NULL;
    // chains_wr.pNext = &chains_emitIRQ;
    chains_wr.DMA_Semaphore = 1;
    chains_wr.DMA_Command = BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);
    chains_wr.DMA_Chain = 0;
    chains_wr.DMA_IRQOnCompletion = 1;
    chains_wr.DMA_NANDLock = 0;
    chains_wr.DMA_NANDWaitForReady = 0;
    chains_wr.DMA_PIOWords = 1;
    chains_wr.pDMABuffer = 00000000;
    chains_wr.DMA_XferBytes = 0000;

    chains_wr.PioWord.B.COUNT = 0000;
    chains_wr.PioWord.B.WORD_LENGTH = 1; // 8bit mode
    chains_wr.PioWord.B.DATA_SELECT = 0; // 0:command mode   1:data mode
    chains_wr.PioWord.B.RUN = 1;
    chains_wr.PioWord.B.BYPASS_COUNT = 0;
    chains_wr.PioWord.B.READ_WRITEB = 0; // 0:write mode  1:read mode
}

static void LCDIF_WriteCMD(uint8_t *dat, uint32_t len)
{
    //while (!LCDIF_checkSendFinish())
        ;
    //while (!LCDIF_checkDMAFin())
        ;
    chains_wr.DMA_Command = BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);
    chains_wr.PioWord.B.DATA_SELECT = 0; // 0:command mode   1:data mode
    chains_wr.PioWord.B.READ_WRITEB = 0; // 0:write mode     1:read mode
    chains_wr.pDMABuffer = (uint32_t)dat;
    chains_wr.DMA_XferBytes = len;
    chains_wr.PioWord.B.COUNT = len;

    BF_WRn(APBH_CHn_NXTCMDAR, 0, CMD_ADDR, (reg32_t)&chains_wr);
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(0, 1);

    // while((!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ)) && (!BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ))  && (!BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ)));
    while ((HW_APBH_CHn_SEMA(0).B.INCREMENT_SEMA))
        ;
    while (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
        ;

    if (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
    {
        INFO("LCDIF ERR IRQ, Overflow:%d, Underflow:%d\n",
             BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ),
             BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ));
        BF_CLR(LCDIF_CTRL1, OVERFLOW_IRQ);
        BF_CLR(LCDIF_CTRL1, UNDERFLOW_IRQ);
    }

    BF_CLR(APBH_CTRL1, CH0_CMDCMPLT_IRQ);

    //while (!LCDIF_checkSendFinish())
        ;
    //while (!LCDIF_checkDMAFin())
        ;
}

static void LCDIF_WriteDAT(uint8_t *dat, uint32_t len)
{
    while (!LCDIF_checkSendFinish())
        ;
    while (!LCDIF_checkDMAFin())
        ;
    chains_wr.DMA_Command = BV_FLD(APBH_CHn_CMD, COMMAND, DMA_READ);
    chains_wr.PioWord.B.DATA_SELECT = 1; // 0:command mode   1:data mode
    chains_wr.PioWord.B.READ_WRITEB = 0; // 0:write mode     1:read mode
    chains_wr.pDMABuffer = (uint32_t)dat;
    chains_wr.DMA_XferBytes = len;
    chains_wr.PioWord.B.COUNT = len;
    BF_WRn(APBH_CHn_NXTCMDAR, 0, CMD_ADDR, (reg32_t)&chains_wr);
    BW_APBH_CHn_SEMA_INCREMENT_SEMA(0, 1);

    while ((HW_APBH_CHn_SEMA(0).B.INCREMENT_SEMA))
        ;
    while (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
        ;
    // while((!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ)) && (!BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ))  && (!BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ)));

    if (!BF_RD(APBH_CTRL1, CH0_CMDCMPLT_IRQ))
    {
        INFO("LCDIF ERR IRQ, Overflow:%d, Underflow:%d\n",
             BF_RD(LCDIF_CTRL1, OVERFLOW_IRQ),
             BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ));
        BF_CLR(LCDIF_CTRL1, OVERFLOW_IRQ);
        BF_CLR(LCDIF_CTRL1, UNDERFLOW_IRQ);
    }

    while (!LCDIF_checkSendFinish())
        ;
    while (!LCDIF_checkDMAFin())
        ;
}

static void LCDIF_Init()
{

    BF_SET(CLKCTRL_CLKSEQ, BYPASS_PIX);
    BF_CLR(CLKCTRL_PIX, CLKGATE);

    BF_CLR(LCDIF_CTRL, SFTRST);
    BF_CLR(LCDIF_CTRL, CLKGATE);

    BF_SET(LCDIF_CTRL, SFTRST);
    while (BF_RD(LCDIF_CTRL, CLKGATE) == 0)
    {
        ;
    }

    BF_CLR(LCDIF_CTRL, SFTRST);
    BF_CLR(LCDIF_CTRL, CLKGATE);


    BF_CLR(CLKCTRL_PIX, CLKGATE);
    BF_CLR(CLKCTRL_FRAC, CLKGATEPIX);
    BF_CLR(CLKCTRL_CLKSEQ, BYPASS_PIX);

    HW_CLKCTRL_PIX_WR(BF_CLKCTRL_PIX_DIV(2)); 
}
DMA_MEM volatile static uint8_t _disp_u8dat;
DMA_MEM volatile static uint32_t _disp_u32dat;
#define LCDIF_CMD8(x)                       \
    do                                      \
    {                                       \
        _disp_u8dat = (x);                           \
        LCDIF_WriteCMD((uint8_t *)&_disp_u8dat, 1);  \
    } while (0)
#define LCDIF_DAT8(x)                       \
    do                                      \
    {                                       \
        _disp_u8dat = (x);                           \
        LCDIF_WriteDAT((uint8_t *)&_disp_u8dat, 1);  \
    } while (0)
#define LCDIF_DAT32(x)                       \
    do                                       \
    {                                        \
        _disp_u32dat = (x);                            \
        LCDIF_WriteDAT((uint8_t *)&_disp_u32dat, 4);   \
    } while (0)

#define BigEnd16(x) ((((x & 0xFFFF) << 8) | ((x & 0xFFFF) >> 8)))

void bsp_diaplay_clean()
{
    uint16_t start_x = SCREEN_START_X;
    uint16_t start_y = 0;
    uint16_t end_x = SCREEN_END_X;
    uint16_t end_y = SCREEN_END_Y;

    LCDIF_CMD8(0x2A);
    LCDIF_DAT32(BigEnd16(start_x) | BigEnd16(end_x) << 16);
    LCDIF_CMD8(0x2B);
    LCDIF_DAT32(BigEnd16(start_y) | BigEnd16(end_y) << 16);
    LCDIF_CMD8(0x2C);

    // for(int i = 0; i < 8; i++){
    //     display_buffer[i] = DISPLAY_INVERSE ? 0xFFFFFFFF :  0x00000000;
    // }
    memset(display_buffer, DISPLAY_INVERSE ? 0xFF : 0, sizeof(display_buffer));
    memset(line_Buffer, DISPLAY_INVERSE ? 0xFF : 0, sizeof(line_Buffer));

    for (int i = 0; i < (((end_x - start_x + 1) * (end_y - start_y + 1)) * 3); i += 32)
    {
        LCDIF_WriteDAT((uint8_t *)line_Buffer, 32);
    }
}

#define BIT8 8

void bsp_display_setbit(int x, int y, int on)
{
    if ((x >= SCREEN_WIDTH) || (y >= SCREEN_HEIGHT))
        return;
    uint8_t *p = &((uint8_t *)display_buffer)[y * SCREEN_WIDTH / BIT8];
    int b = 1 << (x % BIT8);
    p[x / BIT8] &= ~b;
    p[x / BIT8] |= on ? 0 : b;
}

void bsp_display_flush()
{

    LCDIF_CMD8(0x2A);
    LCDIF_DAT32(BigEnd16(0 / 3) | (BigEnd16(255 / 3) << 16));

    for (uint32_t line_i = 8; line_i < 134; line_i++)
    {
        LCDIF_CMD8(0x2B);
        LCDIF_DAT32(BigEnd16(line_i) | ((BigEnd16(line_i) << 16)));

        uint8_t *p = &((uint8_t *)display_buffer)[(line_i - BIT8) * SCREEN_WIDTH / BIT8];

        for (int x = 0; x < 256; x++)
        {
            line_Buffer[x] = ((p[x / BIT8] >> (x % BIT8)) & 1) ? 0xFF : 0;
        }
        LCDIF_CMD8(0x2C);
        LCDIF_WriteDAT(line_Buffer, 256 + 3);
    }
}

void bsp_display_put_string(int x, int y, char *s)
{
    uint32_t str_len = strlen(s);
    uint8_t *font = Ascii1608;
    uint32_t fontWidth = 8;
    int fontSize = 16;
    uint32_t x0 = 0;
    for (int n = 0; n < str_len; n++)
    {
        char c = s[n];
        uint32_t ch = c - ' ';
        uint8_t *p = &font[fontSize * ch];
        for (int y_ = 0; y_ < fontSize; y_++)
        {
            for (int x_ = x0; x_ < x0 + 8; x_++)
            {
                bsp_display_setbit(x + x_,y_ + y, ((p[y_] >> (7 - (x_ - x0))) & 1));
            }
        }
        x0 += fontWidth;
    }
    bsp_display_flush();
}

void bsp_display_init()
{
    LCDIF_DMAChainsInit();
    LCDIF_Init();

    BF_CLRV(APBH_CTRL0, CLKGATE_CHANNEL, 0x1);
    BF_SETV(APBH_CTRL0, RESET_CHANNEL, 0x1);

    BF_CS8(
        PINCTRL_MUXSEL2,
        BANK1_PIN07, 0,
        BANK1_PIN06, 0,
        BANK1_PIN05, 0,
        BANK1_PIN04, 0,
        BANK1_PIN03, 0,
        BANK1_PIN02, 0,
        BANK1_PIN01, 0,
        BANK1_PIN00, 0);

    BF_CS6(
        PINCTRL_MUXSEL3,
        BANK1_PIN21, 0,
        BANK1_PIN20, 0,
        BANK1_PIN19, 0,
        BANK1_PIN18, 0,
        BANK1_PIN17, 0,
        BANK1_PIN16, 0);

    //LCDIFFreq = 24000000;
    LCDIFFreq = 480000000 / HW_CLKCTRL_PIX.B.DIV;

    INFO("LCDIFFreq:%ld\r\n", (uint32_t)LCDIFFreq);

    BF_CS4(LCDIF_TIMING,
           DATA_SETUP, nsToCycles(defaultTiming.DataSetup_ns, 1000000000UL / LCDIFFreq, 1),
           DATA_HOLD, nsToCycles(defaultTiming.DataHold_ns, 1000000000UL / LCDIFFreq, 1),
           CMD_SETUP, nsToCycles(defaultTiming.CmdSetup_ns, 1000000000UL / LCDIFFreq, 1),
           CMD_HOLD, nsToCycles(defaultTiming.CmdHold_ns, 1000000000UL / LCDIFFreq, 1));
    INFO("LCDIF  DATA_SETUP:%d\r\n", BF_RD(LCDIF_TIMING, DATA_SETUP));
    INFO("LCDIF  DATA_HOLD:%d\r\n", BF_RD(LCDIF_TIMING, DATA_HOLD));
    INFO("LCDIF  CMD_SETUP:%d\r\n", BF_RD(LCDIF_TIMING, CMD_SETUP));
    INFO("LCDIF  CMD_HOLD:%d\r\n", BF_RD(LCDIF_TIMING, CMD_HOLD));

    // bsp_enable_irq(HW_IRQ_LCDIF_DMA, true);
    // bsp_enable_irq(HW_IRQ_LCDIF_ERROR, true);
    BF_SET(APBH_CTRL1, CH0_CMDCMPLT_IRQ_EN);
    ;

    BF_CLR(LCDIF_CTRL1, MODE86);
    BF_CLR(LCDIF_CTRL1, LCD_CS_CTRL);

    BF_SET(LCDIF_CTRL1, OVERFLOW_IRQ_EN);
    BF_SET(LCDIF_CTRL1, UNDERFLOW_IRQ_EN);

    BF_SET(LCDIF_CTRL1, BUSY_ENABLE);

    BF_CS1(LCDIF_CTRL1, BYTE_PACKING_FORMAT, 0xF);
    BF_CS1(LCDIF_CTRL1, READ_MODE_NUM_PACKED_SUBWORDS, 4);
    BF_CS1(LCDIF_CTRL1, FIRST_READ_DUMMY, 1);

    BF_CLR(LCDIF_CTRL1, RESET);

    bsp_delayms(120);
    BF_SET(LCDIF_CTRL1, RESET);

    INFO("LCD Init Seq..\r\n");
    LCDIF_CMD8(0xD7); // Auto Load Set
    LCDIF_DAT8(0x9F);
    LCDIF_CMD8(0xE0); // EE Read/write mode
    LCDIF_DAT8(0x00); // Set read mode

    bsp_delayms(100);

    LCDIF_CMD8(0xE3); // Read active
    bsp_delayms(100);

    LCDIF_CMD8(0xE1); // Cancel control
    LCDIF_CMD8(0x11); // sleep out
    bsp_delayms(100);

    LCDIF_CMD8(0x28); // Display off

    LCDIF_CMD8(0xC0); // Set Vop by initial Module
    LCDIF_DAT8(0x01);
    LCDIF_DAT8(0x01);

     LCDIF_CMD8(0xF0); // Set Frame Rate
     LCDIF_DAT32(0x0D0D0D0D);

    LCDIF_CMD8(0xC3); // Bias select
    LCDIF_DAT8(0x04);

    LCDIF_CMD8(0xC4); // Setting Booster times
    LCDIF_DAT8(0x07); //X8: X1 - x8

    LCDIF_CMD8(0xC5); // Booster Efficiency selection
    LCDIF_DAT8(0x03); // Level 3

    LCDIF_CMD8(0xD0); // Analog circuit setting
    LCDIF_DAT8(0x1D);

    LCDIF_CMD8(0xB5); // N-Line
    LCDIF_DAT8(0x8C);
    LCDIF_DAT8(0x00);

    LCDIF_CMD8(0x38); // Idle mode off

    LCDIF_CMD8(0x3A); // pix format
    LCDIF_DAT8(7);

    LCDIF_CMD8(0x36); // Memory Access Control
    LCDIF_DAT8(0x48);

    LCDIF_CMD8(0xB0); // Set Duty
    LCDIF_DAT8(0x89);

    LCDIF_CMD8(0xB4); // Partial Saving Power Mode Selection
    LCDIF_DAT8(0xA0);

    LCDIF_CMD8(0x29); // Display on
    if (DISPLAY_INVERSE)
        LCDIF_CMD8(0x21); // Display inversion on

    LCDIF_CMD8(0x25); // Contrast
    LCDIF_DAT8(28);

    bsp_diaplay_clean();
    bsp_display_flush();
}