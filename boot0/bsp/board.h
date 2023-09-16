#pragma once 
#include <stdint.h>
#include <stdbool.h>

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



extern uint8_t irq_ringbuf[1024];
extern uint16_t irq_pendings;
extern uint16_t irq_pending_ptr;


uint32_t bsp_time_get_us(void);

uint32_t bsp_time_get_ms(void);

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
void bsp_display_put_string(int x, int y, char *s);
void bsp_diaplay_clean();


#include "keyboard.h"

void bsp_keyboard_init();
bool bsp_keyboard_is_key_down(Keys_t key);


void bsp_reset();

