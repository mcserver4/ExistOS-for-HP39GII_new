#pragma once
#include <stdint.h>
#include <stdio.h>

#define USER_MODE 0x10
#define FIQ_MODE 0x11
#define IRQ_MODE 0x12
#define SVC_MODE 0x13
#define ABT_MODE 0x17
#define UND_MODE 0x1b
#define SYS_MODE 0x1f

void print_putc(const char c);
int print_uart0(const char *s) ;

#define INFO(...) do{printf(__VA_ARGS__);}while(0)

void mmu_clean_invalidated_dcache(void * buffer, uint32_t size);

void mmu_clean_dcache(void * buffer, uint32_t size);

void mmu_drain_buffer();

void mmu_invalidate_dcache(void * buffer, uint32_t size);

void mmu_invalidate_tlb();

void mmu_invalidate_icache();

void mmu_invalidate_dcache_all();
uint32_t get_free_mem();


volatile void __attribute__((target("arm"))) switch_mode(int mode) ;
