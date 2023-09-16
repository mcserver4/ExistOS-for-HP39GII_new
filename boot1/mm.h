#pragma once
#include "config.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lfs.h"

#define  PAGE_TYPE_FAULT     (0b00)
#define  PAGE_TYPE_LARGE     (0b01)  //64K
#define  PAGE_TYPE_SMALL     (0b10)  //4K
#define  PAGE_TYPE_TINY      (0b11)  //1K

#define  SECTION_TYPE_FALSE   (0b00)
#define  SECTION_TYPE_COARSE  (0b01)
#define  SECTION_TYPE_SECTION (0b10)
#define  SECTION_TYPE_FINE    (0b11)


#define  AP_SYS_RO__USER_RO    (0b00)
#define  AP_SYS_RW__USER_NONE  (0b01)
#define  AP_SYS_RW__USER_RO    (0b10)
#define  AP_SYS_RW__USER_RW    (0b11)

typedef struct l2_tiny_page_desc_t
{
    unsigned page_type          : 2;
    unsigned buffer             : 1;
    unsigned cache              : 1;
    unsigned AP                 : 2;
    unsigned Reserve            : 4;
    unsigned map_to_addr        : 22;
}l2_tiny_page_desc_t;


typedef struct l1_fine_page_desc_t
{
    unsigned type         : 2;
    unsigned sbz0         : 2;
    unsigned sbo          : 1;
    unsigned domain       : 4;
    unsigned sbz1         : 3;
    unsigned l2_pgt_base  : 20;
}l1_fine_page_desc_t;


typedef struct mmap_table_t
{
    uint32_t map_to_address;
    char *path;
    uint32_t file_offset;
    uint32_t size;
//    l2_tiny_page_desc_t *pgt;
    lfs_file_t *f;
    bool writable;
    bool writeback;
}mmap_table_t;

typedef struct mmap_info
{
    uint32_t map_to;
    const char *path;
    uint32_t offset;
    uint32_t size;
    bool writable;
    bool writeback;
}mmap_info;


void mm_init();

void munmap(uint32_t i);

int mmap(uint32_t to_addr, const char *path, uint32_t offset, uint32_t size, bool writable, bool writeback);

void clean_file_page(uint32_t page);

void mm_set_app_mem_warp(uint32_t svaddr, uint32_t pages);

uint32_t mm_trim_page(uint32_t vaddr);

int do_dab(uint8_t FSR, uint32_t faddr, uint32_t pc, uint8_t faultMode) ;

uint32_t mm_vaddr_map_pa(uint32_t vaddr);

uint32_t is_addr_vaild(uint32_t vaddr, bool writable);
 

uint32_t mm_lock_vaddr(uint32_t vaddr, uint32_t lock);

