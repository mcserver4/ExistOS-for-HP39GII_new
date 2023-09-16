


#include "board.h"
#include "loader.h"
#include "utils.h"


uint32_t bsp_time_get_us(void)
{
    return HW_DIGCTL_MICROSECONDS_RD();
}

uint32_t bsp_time_get_ms(void)
{
    return HW_RTC_MILLISECONDS_RD();
}

uint32_t bsp_time_get_s(void)
{
    return HW_RTC_SECONDS_RD();
}

void bsp_delayms(uint32_t ms)
{
    uint32_t start, cur;
    start = cur = HW_RTC_MILLISECONDS_RD();
    while (cur < start + ms) {
        cur = HW_RTC_MILLISECONDS_RD();
    }
}

void bsp_delayus(uint32_t us)
{
    uint32_t start, cur;
    start = cur = HW_DIGCTL_MICROSECONDS_RD();
    while (cur < start + us) {
        cur = HW_DIGCTL_MICROSECONDS_RD();
    }
}

static void bsp_power_init()
{
    INFO("PWR Init.\n");
    BF_CLR(POWER_CTRL, CLKGATE);



    double DCDC_VDDD =  1.6L;    //360MHz  1.6
    double DCDC_VDDA = 1.9L;    //360MHz  2.0
    double DCDC_VDDIO = 3.5L; //360MHz  3.6
    #warning "FIX ME!"
    /*
    bsp_enable_irq(HW_IRQ_VDD5V, true);
    bsp_enable_irq(HW_IRQ_VDD18_BRNOUT, true);
    bsp_enable_irq(HW_IRQ_VDDD_BRNOUT, true);
    bsp_enable_irq(HW_IRQ_VDDIO_BRNOUT, true);
    bsp_enable_irq(HW_IRQ_BATT_BRNOUT, true);
    bsp_enable_irq(63, true);
    HW_POWER_CTRL.B.ENIRQ_DC_OK = 1;
    HW_POWER_CTRL.B.ENIRQ_LINREG_OK = 1;
    HW_POWER_CTRL.B.ENIRQ_VBUS_VALID = 1;
    HW_POWER_CTRL.B.ENIRQ_VDD5V_GT_VDDIO = 1;
    HW_POWER_CTRL.B.ENIRQ_VDDA_BO = 1;
    HW_POWER_CTRL.B.ENIRQ_VDDD_BO = 1;
    HW_POWER_CTRL.B.ENIRQ_VDDIO_BO = 1;
    */
    HW_POWER_CTRL.B.PSWITCH_IRQ_SRC = 1;

    bsp_delayus(100);
    BF_WR(POWER_DCLIMITS, POSLIMIT_BUCK, 0xF);
    BF_WR(POWER_DCLIMITS, POSLIMIT_BOOST, 0xF);
    BF_WR(POWER_DCLIMITS, NEGLIMIT, 0x6F);
    HW_POWER_DCFUNCV.B.VDDD =  (uint16_t)((DCDC_VDDD * DCDC_VDDA)/((DCDC_VDDA - DCDC_VDDD)*6.25e-3));
    HW_POWER_DCFUNCV.B.VDDIO = (uint16_t)((DCDC_VDDIO * DCDC_VDDA)/((DCDC_VDDIO - DCDC_VDDD)*6.25e-3));
    INFO("VDDD VAL:0x%03x, VDDIO VAL:0x%03x\n",HW_POWER_DCFUNCV.B.VDDD, HW_POWER_DCFUNCV.B.VDDIO);
    

    HW_POWER_5VCTRL.B.EN_BATT_PULLDN = 1;

    HW_POWER_5VCTRL.B.DCDC_XFER = 1;
    HW_POWER_5VCTRL.B.ENABLE_ILIMIT = 0; 
    HW_POWER_LOOPCTRL.B.EN_RCSCALE = 2; 
    HW_POWER_LOOPCTRL.B.DC_C = 0; 


    HW_POWER_5VCTRL.B.OTG_PWRUP_CMPS = 1; //VBUSVALID comparators are enabled
    HW_POWER_5VCTRL.B.VBUSVALID_5VDETECT = 1;
    HW_POWER_5VCTRL.B.ILIMIT_EQ_ZERO = 0;   //short
    
    HW_POWER_5VCTRL.B.PWDN_5VBRNOUT = 0;
    
    HW_POWER_5VCTRL.B.ENABLE_DCDC = 0;


    bsp_delayus(100);
    
    HW_POWER_BATTMONITOR.B.BRWNOUT_LVL = 0; // 0.79 V
    HW_POWER_BATTMONITOR.B.BRWNOUT_PWD = 1;
    HW_POWER_BATTMONITOR.B.EN_BATADJ = 1;
    HW_POWER_BATTMONITOR.B.PWDN_BATTBRNOUT = 1;


    HW_POWER_CHARGE.B.ENABLE_FAULT_DETECT = 0;
    //HW_POWER_CHARGE.B.BATTCHRG_I  = 1 << 5;
    //HW_POWER_CHARGE.B.STOP_ILIMIT = 1 << 3;
    HW_POWER_CHARGE.B.USE_EXTERN_R = 1;

    HW_POWER_CHARGE.B.CHRG_STS_OFF = 0;
    HW_POWER_CHARGE.B.PWD_BATTCHRG = 1;

    bsp_delayus(500);
    HW_POWER_VDDDCTRL.B.ENABLE_LINREG = 1;
    HW_POWER_VDDACTRL.B.ENABLE_LINREG = 1;
        

    HW_POWER_VDDDCTRL.B.DISABLE_FET = 1;
    HW_POWER_VDDDCTRL.B.LINREG_OFFSET = 0; 
    HW_POWER_VDDDCTRL.B.ALKALINE_CHARGE = 0;

    BF_WR(POWER_VDDDCTRL, TRG,  (uint8_t)((1.43 - 0.8)/0.025) );  // val = (TAG_v - 0.8v)/0.025v, 0.8~1.45, 1.2
    bsp_delayus(200);
    BF_WR(POWER_VDDACTRL, TRG,  (uint8_t)((1.75 - 1.5)/0.025) );  // val = (TAG_v - 1.5v)/0.025v, 1.5~1.95, 1.75
    bsp_delayus(200);
    BF_WR(POWER_VDDIOCTRL, TRG, (uint8_t)((3.3 - 2.8)/0.025));  // val = (TAG_v - 2.8v)/0.025v, 2.8~3.575, 3.1
    bsp_delayus(100);

    INFO("VDDIO LDO VAL:0x%03x\n", HW_POWER_VDDIOCTRL.B.TRG );
    INFO("VDDD LDO VAL:0x%03x\n", HW_POWER_VDDDCTRL.B.TRG );
    INFO("VDDA LDO VAL:0x%03x\n", HW_POWER_VDDACTRL.B.TRG );

}

static void bsp_timer0_init()
{
    BF_CLR(TIMROT_ROTCTRL, SFTRST);
    BF_CLR(TIMROT_ROTCTRL, CLKGATE);
    
    BF_SET(TIMROT_ROTCTRL, SFTRST);
    while(BF_RD(TIMROT_ROTCTRL, CLKGATE) == 0)
        ;
    
    BF_CLR(TIMROT_ROTCTRL, SFTRST);
    BF_CLR(TIMROT_ROTCTRL, CLKGATE);
    
    BF_WRn(TIMROT_TIMCTRLn, 0, SELECT, BV_TIMROT_TIMCTRLn_SELECT__32KHZ_XTAL);
}

void bsp_timer0_set(uint32_t us)
{
    if(us)
    {
        BF_CLRn(TIMROT_TIMCTRLn, 0, IRQ);
        bsp_enable_irq(HW_IRQ_TIMER0, true);
        BF_CS1n(TIMROT_TIMCOUNTn, 0, FIXED_COUNT, us/32);
        BF_CS1n(TIMROT_TIMCTRLn, 0, IRQ_EN, true);
        BF_CS1n(TIMROT_TIMCTRLn, 0, RELOAD, true);
        BF_CS1n(TIMROT_TIMCTRLn, 0, UPDATE, true);

    }else{
        bsp_enable_irq(HW_IRQ_TIMER0, false);
        BF_CS1n(TIMROT_TIMCTRLn, 0, IRQ_EN, false);
        BF_CS1n(TIMROT_TIMCTRLn, 0, RELOAD, false);
        BF_CS1n(TIMROT_TIMCTRLn, 0, UPDATE, false);
        BF_CS1n(TIMROT_TIMCOUNTn,0, FIXED_COUNT, 0);
        BF_CLRn(TIMROT_TIMCTRLn, 0, IRQ);
    }
}




void bsp_dma_init()
{
    BF_CLR(APBH_CTRL0, SFTRST);
    BF_CLR(APBH_CTRL0, CLKGATE);

    BF_SET(APBH_CTRL0, SFTRST);
    while(BF_RD(APBH_CTRL0, CLKGATE) == 0){
        ;
    }

    BF_CLR(APBH_CTRL0, SFTRST);
    BF_CLR(APBH_CTRL0, CLKGATE);
    
    BF_CLR(APBX_CTRL0, SFTRST);
    BF_CLR(APBX_CTRL0, CLKGATE);

    BF_SET(APBX_CTRL0, SFTRST);
    while(BF_RD(APBX_CTRL0, CLKGATE) == 0){
        ;
    }

    BF_CLR(APBX_CTRL0, SFTRST);
    BF_CLR(APBX_CTRL0, CLKGATE);
}

static void bsp_usbphy_init()
{
    BF_CLR(USBPHY_CTRL, SFTRST);
    BF_CLR(USBPHY_CTRL, CLKGATE);

    BF_SET(USBPHY_CTRL, SFTRST);
    while(BF_RD(USBPHY_CTRL, CLKGATE) == 0)
    {
        ;
    }

    BF_CLR(USBPHY_CTRL, SFTRST);
    BF_CLR(USBPHY_CTRL, CLKGATE);
}

void bsp_usb_phy_enable()
{
    BF_SET(CLKCTRL_PLLCTRL0, EN_USB_CLKS);
    HW_USBPHY_PWD_CLR(0xffffffff);
    BF_CLR(DIGCTL_CTRL, USB_CLKGATE);

}

void bsp_usb_dcd_int_enable(uint32_t enable)
{
    bsp_enable_irq(HW_IRQ_USB_CTRL, enable);
}

void bsp_reset()
{
    BF_WR(CLKCTRL_RESET, CHIP, 1);
}

void bsp_board_init()
{
  
    bsp_irq_init();
    bsp_power_init();
    bsp_clksource_init();

    bsp_dma_init();
    bsp_usbphy_init();
    bsp_timer0_init();
    
    bsp_nand_init();

    bsp_display_init();
    bsp_keyboard_init();
}
