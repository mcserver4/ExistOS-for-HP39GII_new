


#include "board.h" 
#include "utils.h"


uint32_t bsp_time_get_us(void)
{
    return HW_DIGCTL_MICROSECONDS_RD();
}

uint32_t bsp_time_get_ms(void)
{
    return HW_RTC_MILLISECONDS_RD();
}

void bsp_time_reset_mstick()
{
    INFO("Reset ms tick\r\n");
    HW_RTC_MILLISECONDS_CLR(BM_RTC_MILLISECONDS_COUNT);
}

uint32_t bsp_rtc_get_s(void)
{
    return HW_RTC_SECONDS_RD();
}

void bsp_rtc_set_s(uint32_t t)
{
    HW_RTC_SECONDS_WR(t);
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
   
 
 
}
