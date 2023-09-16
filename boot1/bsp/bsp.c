#include "bsp.h"

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


