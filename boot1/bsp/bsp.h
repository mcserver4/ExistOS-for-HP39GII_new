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


uint32_t bsp_time_get_us(void);

uint32_t bsp_time_get_ms(void);

uint32_t bsp_time_get_s(void);

void bsp_delayms(uint32_t ms);

void bsp_delayus(uint32_t us);

void bsp_nand_init(uint32_t *dma_chain_info);

void bsp_nand_write_page(uint32_t page, uint8_t *dat);
void bsp_nand_write_page_nometa(uint32_t page, uint8_t *dat);
void bsp_nand_erase_block(uint32_t block);

int bsp_nand_read_page(uint32_t page, uint8_t *buffer, uint32_t *ECC);
int bsp_nand_read_page_nometa(uint32_t page, uint8_t *buffer, uint32_t *ECC);

bool bsp_nand_check_is_bad_block(uint32_t block);

uint32_t* bsp_nand_getid();

