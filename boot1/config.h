#pragma once

//#define printf(...)

#include <stdint.h>

extern int pgt_size;
extern int page_frame_size;

#define PAGE_SIZE               (1024)  //TINY PAGE
#define PGT_SIZE                (4096)  //TINY PAGE
#define PGT_ITEMS               (PGT_SIZE / 4)  //TINY PAGE
#define PAGES_PER_SECTION       (1048576/PAGE_SIZE) //TINY PAGE

#define MAX_RESERVE_FILE_PAGES  (0)

#define PAGE_FRAMES                 ((uint32_t)&page_frame_size / PAGE_SIZE)
#define MAX_ALLOW_MMAP_FILE         (((uint32_t)&pgt_size / PGT_SIZE) - 1)

#define BOOT1_HEAD_LOAD_ADDR     (0xD0001000)
#define BOOT1_BODY_OFFSET   (0x50)

#define MAIN_MEMORY_BASE    (0x02000000)
#define APP_MEMORY_BASE     (0x02080000)
#define SYSTEM_LOAD_ADDR    (0x00100000)

#define FS_CACHE_SZ         (2048)
extern int __HEAP_START;
extern int __BOOT1_HEAD_END;

#define PA(x)   (((uint32_t)x) & 0x000FFFFF)
#define VA(x)   (((uint32_t)x) | 0xD0000000)

#define ABT_STACK_SZ        (256)
#define UND_STACK_SZ        (256)
#define IRQ_STACK_SZ        (256)
#define FIQ_STACK_SZ        (16)
#define SYS_STACK_SZ        (256)
#define SVC_STACK_SZ        (2048+512)



#define ABT_STACK_VADDR      ((uint32_t)&__BOOT1_HEAD_END - 4)
#define UND_STACK_VADDR      (ABT_STACK_VADDR - ABT_STACK_SZ)
#define FIQ_STACK_VADDR      (UND_STACK_VADDR - UND_STACK_SZ)
#define IRQ_STACK_VADDR      (FIQ_STACK_VADDR - FIQ_STACK_SZ)
#define SYS_STACK_VADDR      (IRQ_STACK_VADDR - IRQ_STACK_SZ)
#define SVC_STACK_VADDR      (SYS_STACK_VADDR - SYS_STACK_SZ)


#define HEAP_END  (SVC_STACK_VADDR - SVC_STACK_SZ)
 

#include "lfs.h"
extern lfs_t lfs;
