#pragma once 
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "utils.h"

#include "regs.h"
#include "hw_irq.h"
#include "regsicoll.h"
#include "regspower.h"
#include "regslradc.h"
#include "regsapbh.h"
#include "regsapbx.h"
#include "regsgpmi.h"
#include "regspinctrl.h"
#include "regsecc8.h"
#include "regsusbphy.h"
#include "regsdigctl.h"
#include "regslcdif.h"
#include "regstimrot.h"
#include "regsclkctrl.h"
#include "regspower.h"
#include "regsrtc.h"
#include "regslradc.h"

#define DMA_MEM  __attribute__ (( section(".dma_ram") ))


#define INFO(...) do{printf(__VA_ARGS__);}while(0)

uint32_t bsp_time_get_us(void);

uint32_t bsp_time_get_ms(void);
void bsp_time_reset_mstick();

uint32_t bsp_rtc_get_s(void);

void bsp_rtc_set_s(uint32_t t);

uint32_t bsp_time_get_s(void);

void bsp_delayms(uint32_t ms);

void bsp_delayus(uint32_t us);

void bsp_irq_init();

void bsp_enable_irq(uint32_t IRQNum, uint32_t enable);
 

void do_irq();

void bsp_timer0_set(uint32_t us);

void bsp_usb_phy_enable();

void bsp_usb_dcd_int_enable(uint32_t enable);

void bsp_board_init();

void bsp_clksource_init();

void bsp_nand_write_page_nometa(uint32_t page, uint8_t * dat);
int bsp_nand_read_page_no_meta(uint32_t page, uint8_t *buffer, uint32_t *ECC);

void bsp_nand_init();
int bsp_nand_read_page(uint32_t page, uint8_t *buffer, uint32_t *ECC);
void bsp_nand_erase_block(uint32_t block);
void bsp_nand_write_page(uint32_t page, uint8_t *dat);
bool bsp_nand_check_is_bad_block(uint32_t block);

void bsp_display_init();
void bsp_display_putk_string(uint32_t x, uint32_t y, char *s, uint32_t fg, uint32_t bg);
void bsp_diaplay_clean(uint32_t c);
void bsp_display_set_point(uint32_t x, uint32_t y, uint32_t c);
uint32_t bsp_display_get_point(uint32_t x, uint32_t y);
void bsp_diaplay_put_hline(uint32_t y, void *dat);
void bsp_display_flush();

void bsp_reset();


#include "keyboard.h"


