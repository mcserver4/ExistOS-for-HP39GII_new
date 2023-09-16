#pragma once

#include <stdint.h>
#include <stdbool.h>


uint32_t xPortGetAllocatedPages();

uint32_t xPortGetFreePages();

uint32_t xPortHeapTrimFreePage();

#define USER_MODE 0x10
#define FIQ_MODE 0x11
#define IRQ_MODE 0x12
#define SVC_MODE 0x13
#define ABT_MODE 0x17
#define UND_MODE 0x1b
#define SYS_MODE 0x1f


void __attribute__((naked)) __attribute__((target("arm"))) sys_svc();
void __attribute__((naked)) __attribute__((target("arm"))) sys_irq();
void __attribute__((naked)) __attribute__((target("arm"))) sys_dab();
void __attribute__((naked)) __attribute__((target("arm"))) sys_pab();


int print_uart0(const char *s) ;
void print_putc(const char c);

volatile void __attribute__((target("arm"))) switch_mode(int mode) ;

uint32_t nsToCycles(uint64_t nstime, uint64_t period, uint64_t min) ;


void __attribute__((target("arm"))) mmu_clean_dcache(void * buffer, uint32_t size);
void __attribute__((target("arm"))) mmu_invalidate_icache();
void __attribute__((target("arm"))) mmu_invalidate_dcache_all();
void __attribute__((target("arm"))) mmu_invalidate_tlb();
void __attribute__((target("arm"))) mmu_invalidate_dcache(void * buffer, uint32_t size);
void __attribute__((target("arm"))) mmu_drain_buffer();
void __attribute__((target("arm"))) mmu_clean_invalidated_dcache(void *buffer, uint32_t size);


void memTestWrite(uint32_t base, uint32_t length, uint32_t seed);
void memTestRead(uint32_t base, uint32_t length, bool trim, uint32_t seed);


