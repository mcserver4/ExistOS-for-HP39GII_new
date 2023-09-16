
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>

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

uint32_t nsToCycles(uint64_t nstime, uint64_t period, uint64_t min) 
{
    uint32_t k;
    k = (nstime + period - 1) / period;
    return (k > min) ? k : min;
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








uint32_t _randSeed = 1;

uint32_t _rand(void)
{
    uint32_t value;
    //Use a linear congruential generator (LCG) to update the state of the PRNG
    _randSeed *= 1103515245;
    _randSeed += 12345;
    value = (_randSeed >> 16) & 0x07FF;
    _randSeed *= 1103515245;
    _randSeed += 12345;
    value <<= 10;
    value |= (_randSeed >> 16) & 0x03FF;
    _randSeed *= 1103515245;
    _randSeed += 12345;
    value <<= 10;
    value |= (_randSeed >> 16) & 0x03FF;
    //Return the random value
    return value;
}



void memTestWrite(uint32_t base, uint32_t length, uint32_t seed)
{
    uint32_t val;
    uint8_t *p = (uint8_t *)base;
    
    _randSeed = seed;
    printf("memtest start\r\n");

    for(int i = 0; i < length;i++)
    {
        val = _rand() & 0xFF;        
        if(i < 20)
        {
            printf("TST WR:%02lX\r\n", val);
        }
        p[i] = val;
    }
    
    printf("memtest write fin\r\n");
}

void memTestRead(uint32_t base, uint32_t length, bool trim, uint32_t seed)
{
    uint32_t val;
    uint8_t *p = (uint8_t *)base; 
    
    _randSeed = seed;
    for(int i = 0; i < length;i++)
    {
        val = _rand() & 0xFF;
        if(i < 20)
        {
            printf("TST RD:%02lX\r\n", val);
        }
        if(p[i] != val)
        {
            printf("MEM ERR:%p, %lX == %X\r\n",&p[i],val,p[i]);
        }
    }

    //if(trim)
        //for(int i = 0; i < length;i+=1024)
            //mm_trim_page((uint32_t)&p[i]); 
 

    printf("memtest fin\r\n");
}


