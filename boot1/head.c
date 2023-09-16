#include <stdint.h>
#include "config.h"
#include "utils.h"

volatile void __init() __attribute__((naked)) __attribute__((target("arm"))) __attribute__((section(".init"))) ;
volatile void __init() 
{

    __asm volatile("mrs r1, cpsr_all");
    __asm volatile("orr r1, r1, #0xc0");//Disable interrupt
    __asm volatile("orr r1, r1, #0x13");//SVC MODE
    __asm volatile("msr cpsr_all, r1");	
    
    __asm volatile("mov r4,r0");
	__asm volatile("ldr r0,=0xD0070060");
	__asm volatile("ldr r1,=0xD0080000");
	__asm volatile("ldr r2,=0xD0002000"); //copy 0xD0070060~0xD0080000  to >0xD0002000
	__asm volatile("_head_copy:");
	__asm volatile("ldr r3,[r0]");
	__asm volatile("str r3,[r2]");
	__asm volatile("add r0,r0,#4");
	__asm volatile("add r2,r2,#4");
	__asm volatile("cmp r0,r1");
    __asm volatile("blo _head_copy");
    __asm volatile("mov r0,r4");
    __asm volatile("b boot");
}

volatile void __attribute__((naked)) __attribute__((target("arm"))) vector()
{
	__asm volatile("ldr pc,.Lreset");
	__asm volatile("ldr pc,.Lund");
	__asm volatile("ldr pc,.Lsvc");
	__asm volatile("ldr pc,.Lpab");
	__asm volatile("ldr pc,.Ldab");
	__asm volatile("ldr pc,.Lreset");
	__asm volatile("ldr pc,.Lirq");
	__asm volatile("ldr pc,.Lfiq");

	__asm volatile(".Lreset: 	.long 	 reboot");
	__asm volatile(".Lund: 		.long 	 arm_vector_und");
	__asm volatile(".Lsvc: 		.long 	 arm_vector_swi");
	__asm volatile(".Lpab: 		.long 	 arm_vector_pab");
	__asm volatile(".Ldab: 		.long 	 arm_vector_dab");
	__asm volatile(".Lirq: 		.long 	 arm_vector_irq");
	__asm volatile(".Lfiq: 		.long 	 arm_vector_fiq");
}

volatile void __attribute__((naked)) __attribute__((target("arm"))) asm_wr(uint32_t addr, uint32_t val)
{
	__asm volatile("str r1,[r0]");
	__asm volatile("bx lr");
}

extern unsigned int __bss_start;
extern unsigned int __bss_end;

extern int main();
void  __attribute__((naked)) __attribute__((target("arm"))) reboot()
{
	print_uart0("Reboot!\r\n");
    while(1);
}

volatile void set_stack(unsigned int newstackptr) __attribute__((naked)); __attribute__((target("arm")))
volatile void set_stack(unsigned int newstackptr) {
    __asm volatile("mov sp,r0");
    __asm volatile("bx lr");
}


volatile void switch_mode(int mode) __attribute__((naked));

void  __attribute__((naked)) __attribute__((target("arm"))) boot(uint32_t par)
{
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

	for(int i = 0; i < 15; i ++)
	{
		asm_wr(i*4, ((uint32_t *)vector)[i]);	
	}

    main(par);

    for(;;)
    ;
}


