#include "loader.h"
#include <stdint.h>
#include "utils.h"

#define SECTION_DESC(SEG, ap, c, b, region)  ((SEG<<20)|(ap << 10)|(region << 5)|(c<<3)|(b<<2)|(1<<4)|(1<<1))

volatile void __attribute__((naked)) __attribute__((target("arm")))vector()
{
	__asm volatile("ldr pc,.Lreset");
	__asm volatile("ldr pc,.Lund");
	__asm volatile("ldr pc,.Lsvc");
	__asm volatile("ldr pc,.Lpab");
	__asm volatile("ldr pc,.Ldab");
	__asm volatile("ldr pc,.Lreset");
	__asm volatile("ldr pc,.Lirq");
	__asm volatile("ldr pc,.Lfiq");

	__asm volatile(".Lreset: 	.long 	 _boot");
	__asm volatile(".Lund: 		.long 	 arm_vector_und");
	__asm volatile(".Lsvc: 		.long 	 arm_vector_swi");
	__asm volatile(".Lpab: 		.long 	 arm_vector_pab");
	__asm volatile(".Ldab: 		.long 	 arm_vector_dab");
	__asm volatile(".Lirq: 		.long 	 arm_vector_irq");
	__asm volatile(".Lfiq: 		.long 	 arm_vector_fiq");
}

volatile void __init() __attribute__((naked)) __attribute__((target("arm"))) __attribute__((section(".init"))) ;
volatile void __init() 
{

	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 
	__asm volatile("nop"); 

    __asm volatile("mrs r1, cpsr_all");
    __asm volatile("orr r1, r1, #0xc0");//Disable interrupt
    __asm volatile("orr r1, r1, #0x13");//SVC MODE
    __asm volatile("msr cpsr_all, r1");	

	__asm volatile("ldr r13,=0x0080000");
	volatile uint32_t *tmp_pg = (uint32_t *)TMP_PGT_PADDR;
	tmp_pg[BOOT0_MEM_BASE >> 20] = SECTION_DESC(0, 3, 0, 0, 0);  
	tmp_pg[0x800] = SECTION_DESC(0x800, 3, 0, 0, 0);
	tmp_pg[0] = SECTION_DESC(0, 3, 0, 0, 0);
	
	__asm volatile("ldr r0,=%0"::""(TMP_PGT_PADDR));
	__asm volatile("mcr p15, 0, R0, c2, c0, 0");
	
	
	__asm volatile("ldr r0,=0x1");
	__asm volatile("mcr p15, 0, R0, c3, c0, 0"); 

	//!!! ERROR IN STMP3770
	//__asm volatile ("mov r0,#0");
	//__asm volatile ("mcr p15, 0, r0, c8, c7, 0");
	//__asm volatile ("mcr p15, 0, r0, c7, c5, 0");
	//__asm volatile ("mcr p15, 0, r0, c7, c6, 0");
	//__asm volatile ("mcr p15, 0, r0, c7, c10, 0");


	__asm volatile("mrc p15, 0, R0, c1, c0, 0");
	__asm volatile("bic R0,R0,#0x2000"); //Exception vector at 0
	//__asm volatile("bic R0,R0,#0x4");	 //Disable Cache
	__asm volatile("orr R0,R0,#0x4");	 //Enable Cache
	__asm volatile("orr R0,R0,#0x1");	 //ENABLE MMU
	
	__asm volatile("bic R0,R0,#0x100"); //CLR S
	__asm volatile("orr R0,R0,#0x200"); //SET R

	__asm volatile("mcr p15, 0, R0, c1, c0, 0"); 

	__asm volatile("nop"); 
	__asm volatile("nop"); 

	__asm volatile("ldr r0,=_boot"); 
	__asm volatile("bx r0");

}

volatile void __attribute__((naked)) __attribute__((target("arm"))) asm_wr(uint32_t addr, uint32_t val)
{
	__asm volatile("str r1,[r0]");
	__asm volatile("bx lr");
}

volatile void set_stack(unsigned int newstackptr) __attribute__((naked)); __attribute__((target("arm")))
volatile void set_stack(unsigned int newstackptr) {
    __asm volatile("mov sp,r0");
    __asm volatile("bx lr");
}

extern unsigned int __bss_start;
extern unsigned int __bss_end;   


#include "regsdigctl.h"
static inline void SetMPTELoc(uint32_t mpte, uint32_t seg)
{
    BF_CS1n(DIGCTL_MPTEn_LOC, mpte, LOC, seg);
}


volatile void _boot()  __attribute__((naked)); __attribute__((target("arm")))
volatile void _boot(){
	  

	for(char *i = (char *)&__bss_start; i < (char *)&__bss_end; i++){
		*i = 0;		//clear bss
	}
	
    switch_mode(ABT_MODE);
    set_stack(ABT_STACK_VADDR);

    switch_mode(UND_MODE); 
    set_stack(UND_STACK_VADDR);

    switch_mode(FIQ_MODE);
    set_stack(FIQ_STACK_VADDR); 

    switch_mode(IRQ_MODE);
    set_stack(IRQ_STACK_VADDR);

    switch_mode(SYS_MODE);
    set_stack(SYS_STACK_VADDR);

    switch_mode(SVC_MODE);
	set_stack(SVC_STACK_VADDR);

	print_uart0("BOOT0START\r\n");

	for(int i = 0; i < 15; i ++)
	{
		asm_wr(i*4, ((uint32_t *)vector)[i]);	
	}

	SetMPTELoc(0,0);
	SetMPTELoc(1,BOOT0_MEM_BASE >> 20);

	volatile uint32_t *tmp_pg = (uint32_t *)0x800C0000;
	tmp_pg[BOOT0_MEM_BASE >> 20] = SECTION_DESC(0, 3, 0, 0, 0);  
	tmp_pg[0] = SECTION_DESC(0, 3, 0, 0, 0);
	__asm volatile("ldr r0,=%0"::""(0x800C0000));
	__asm volatile("mcr p15, 0, R0, c2, c0, 0");
	mmu_invalidate_tlb();


	
	__asm volatile("b main");
}
