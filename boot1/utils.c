
#include "utils.h"


#define CACHE_LINE_SIZE     32

volatile unsigned int *const UART0DR = (unsigned int *)0x80070000;
volatile unsigned int *const UART0REG = (unsigned int *)0x80070018;



void print_putc(const char c) 
{
  while (*UART0REG & (1 << 3)) {
    ;
  }
  *UART0DR = c;
}

int print_uart0(const char *s) 
{
    int i = 0;
  while (*s != '\0') { /* Loop until end of string */
    print_putc(*s);    /* Transmit char */
    s++;               /* Next char */
    i++;
  }
  return i;
}


void __attribute__((target("arm"))) mmu_clean_invalidated_dcache(void *buffer, uint32_t size)
{
    register uint32_t ptr;

    ptr = (uint32_t)buffer & ~(CACHE_LINE_SIZE - 1);

    while (ptr < (uint32_t)buffer + size)
    {
        __asm volatile ( "MCR p15, 0, %0, c7, c14, 1" :: "r"(ptr) );
        ptr += CACHE_LINE_SIZE;
    }
}


void __attribute__((target("arm"))) mmu_clean_dcache(void * buffer, uint32_t size)
{
    register uint32_t ptr;

    ptr = (uint32_t)buffer & ~(CACHE_LINE_SIZE - 1);

    while (ptr < (uint32_t)buffer + size)
    {
        __asm volatile ("MCR p15, 0, %0, c7, c10, 1 " :: "r"(ptr) );
        ptr += CACHE_LINE_SIZE;
    }
}

void __attribute__((target("arm"))) mmu_drain_buffer()
{
    register uint32_t reg;
    reg = 0;
    __asm volatile("mcr p15,0,%0,c7,c10,4" :: "r"(reg));


}

void __attribute__((target("arm"))) mmu_invalidate_dcache(void * buffer, uint32_t size)
{
    register uint32_t ptr;

    ptr = (uint32_t)buffer & ~(CACHE_LINE_SIZE - 1);

    while (ptr < (uint32_t)buffer + size)
    {
        __asm volatile ("MCR p15, 0, %0, c7, c6, 1" :: "r"(ptr) );
        ptr += CACHE_LINE_SIZE;
    }
}

void __attribute__((target("arm"))) mmu_invalidate_tlb()
{
    register uint32_t value asm("r0");

    value = 0;
    __asm volatile ("mcr p15, 0, %0, c8, c7, 0" :: "r"(value) );

}

void __attribute__((target("arm"))) mmu_invalidate_icache()
{
    register uint32_t value asm("r0");

    value = 0;

    __asm volatile ("mcr p15, 0, %0, c7, c5, 0" :: "r"(value) );
}


void __attribute__((target("arm"))) mmu_invalidate_dcache_all()
{
    register uint32_t value asm("r0");

    value = 0;

    __asm volatile (" mcr p15, 0, %0, c7, c6, 0 " :: "r"(value) );
}

volatile void __attribute__((target("arm"))) switch_mode(int mode) 
 {
  __asm volatile("and r0,r0,#0x1f");
  __asm volatile("mrs r1,cpsr_all");
  __asm volatile("bic r1,r1,#0x1f");
  __asm volatile("orr r1,r1,r0");
  __asm volatile("mov r0,lr"); // GET THE RETURN ADDRESS **BEFORE** MODE CHANGE
  __asm volatile("msr cpsr_all,r1");
  __asm volatile("bx r0");
}


