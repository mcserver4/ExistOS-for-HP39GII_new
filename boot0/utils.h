#pragma once

#include <stdint.h>

#define USER_MODE 0x10
#define FIQ_MODE 0x11
#define IRQ_MODE 0x12
#define SVC_MODE 0x13
#define ABT_MODE 0x17
#define UND_MODE 0x1b
#define SYS_MODE 0x1f


void print_uart0(const char *s);
void cpu_enable_irq();
void cpu_disable_irq();
void print_putc(const char c);

volatile void switch_mode(int mode) __attribute__((naked));
uint32_t __attribute__((target("arm"))) get_mode();

#define INFO(...) do{printf(__VA_ARGS__);}while(0)

void mmu_clean_invalidated_dcache(uint32_t buffer, uint32_t size);

void mmu_clean_dcache(uint32_t buffer, uint32_t size);

void mmu_drain_buffer();

void mmu_invalidate_dcache(uint32_t buffer, uint32_t size);

void mmu_invalidate_tlb();

void mmu_invalidate_icache();

void mmu_invalidate_dcache_all();

uint32_t nsToCycles(uint64_t nstime, uint64_t period, uint64_t min) ;

uint32_t get_cpu_id();
void jump_boot1();
