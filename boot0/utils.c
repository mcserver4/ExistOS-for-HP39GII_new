#include "utils.h"
#define CACHE_LINE_SIZE     32

volatile unsigned int *const UART0DR = (unsigned int *)0x80070000;
volatile unsigned int *const UART0REG = (unsigned int *)0x80070018;


void __attribute__((target("arm"))) mmu_clean_invalidated_dcache(uint32_t buffer, uint32_t size)
{
    register uint32_t ptr;

    ptr = buffer & ~(CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
    {
        __asm volatile ( "MCR p15, 0, %0, c7, c14, 1" :: "r"(ptr) );
        ptr += CACHE_LINE_SIZE;
    }
}


void __attribute__((target("arm"))) mmu_clean_dcache(uint32_t buffer, uint32_t size)
{
    register uint32_t ptr;

    ptr = buffer & ~(CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
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

void __attribute__((target("arm"))) mmu_invalidate_dcache(uint32_t buffer, uint32_t size)
{
    register uint32_t ptr;

    ptr = buffer & ~(CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
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

static uint32_t irq_lock = 0;

void __attribute__((target("arm"))) cpu_disable_irq()
{
  if(irq_lock == 0)
  {
    __asm volatile("mrs r1, cpsr_all");
    __asm volatile("orr r1, r1, #0xc0");//disable interrupt
    __asm volatile("msr cpsr_all, r1");	
  }
  irq_lock++;
}
void __attribute__((target("arm"))) cpu_enable_irq()
{
  if(irq_lock){
    irq_lock--;
  }
  if(irq_lock == 0)
  {
    __asm volatile("mrs r1, cpsr_all");
    __asm volatile("bic r1, r1, #0xc0");//enable interrupt
    __asm volatile("msr cpsr_all, r1");	
  }
}

void __malloc_lock()
{
  cpu_disable_irq();
}

void __malloc_unlock()
{
  cpu_enable_irq();
}

void print_putc(const char c) {
  while (*UART0REG & (1 << 3)) {
    ;
  }
  *UART0DR = c;
}
void print_uart0(const char *s) {
  while (*s != '\0') { /* Loop until end of string */
    print_putc(*s);    /* Transmit char */
    s++;               /* Next char */
  }
}

volatile void __attribute__((target("arm"))) switch_mode(int mode)  {
  __asm volatile("and r0,r0,#0x1f");
  __asm volatile("mrs r1,cpsr_all");
  __asm volatile("bic r1,r1,#0x1f");
  __asm volatile("orr r1,r1,r0");
  __asm volatile("mov r0,lr"); // GET THE RETURN ADDRESS **BEFORE** MODE CHANGE
  __asm volatile("msr cpsr_all,r1");
  __asm volatile("bx r0");
}

uint32_t __attribute__((target("arm"))) get_mode()
{
  register uint32_t r0 = 0;
  __asm volatile("mrs %0,cpsr_all":"=r"(r0));
  return r0;
}

uint32_t __attribute__((target("arm"))) get_cpu_id()
{
  register uint32_t r0 = 0;
  __asm volatile("mrc p15, 0, %0, c0, c0, 0"
                 : "=r"(r0));
  return r0;
}

#include "loader.h"
void __attribute__((target("arm"))) jump_boot1()
{
  __asm volatile("ldr r1,=%0"::""(BOOT1_LOAD_ADDR));
  __asm volatile("bx r1");
}

uint32_t nsToCycles(uint64_t nstime, uint64_t period, uint64_t min) 
{
    uint32_t k;
    k = (nstime + period - 1) / period;
    return (k > min) ? k : min;
}

