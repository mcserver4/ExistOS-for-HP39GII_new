#pragma once

#include <stdint.h>
#include <stdio.h>


#define BOOT1_SIZE    (64 * 1024)

#define BOOT0_MEM_BASE       (0xD0000000)
#define BOOT0_MEM_SIZE        (512 * 1024 - BOOT1_SIZE)

#define BOOT1_LOAD_ADDR    (BOOT0_MEM_BASE + BOOT0_MEM_SIZE) //0xD0070000


#define TMP_PGT_PADDR      (0x64000)

#define ABT_STACK_VADDR      (BOOT0_MEM_BASE  + BOOT0_MEM_SIZE - 4)
#define UND_STACK_VADDR      (ABT_STACK_VADDR - 16)
#define FIQ_STACK_VADDR      (UND_STACK_VADDR - 4096)
#define IRQ_STACK_VADDR      (FIQ_STACK_VADDR - 16)
#define SYS_STACK_VADDR      (IRQ_STACK_VADDR - 8192)
#define SVC_STACK_VADDR      (SYS_STACK_VADDR - 16)

extern unsigned int __HEAP_START;
#define HEAP_END  (SVC_STACK_VADDR - 8192)

#define DMA_MEM   __attribute__((section(".dma_mem")))
#define DMA_MEM_CHAINS   __attribute__((section(".dma_mem.chains")))


