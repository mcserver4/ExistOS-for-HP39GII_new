
#include "utils.h"   
#include <stdio.h>

#include "board.h"

unsigned int faultAddress;
volatile unsigned int insAddress;
unsigned int FSR;

void volatile __attribute__((target("arm"))) __attribute__((naked)) arm_vector_und()
{
    print_uart0("und\r\n");

    while(1);
}

void volatile __attribute__((target("arm"))) __attribute__((naked)) arm_vector_swi()
{

    print_uart0("swi\r\n");
    while(1);
}

void volatile __attribute__((target("arm"))) __attribute__((naked)) arm_vector_pab()
{
    __asm volatile("PUSH	{R0}");
    __asm volatile("mrc p15, 0, r0, c6, c0, 0");
    __asm volatile("str r0,%0"
                   : "=m"(faultAddress));
    __asm volatile("mrc p15, 0, r0, c5, c0, 0");
    __asm volatile("str r0,%0"
                   : "=m"(FSR));
    __asm volatile("POP	{R0}");


    print_uart0("pab\r\n");
    while(1);

}

void volatile __attribute__((target("arm"))) __attribute__((naked)) arm_vector_dab()
{
    __asm volatile("PUSH	{R0}");
    __asm volatile("mrc p15, 0, r0, c6, c0, 0");
    __asm volatile("str r0,%0"
                   : "=m"(faultAddress));
    __asm volatile("mrc p15, 0, r0, c5, c0, 0");
    __asm volatile("str r0,%0"
                   : "=m"(FSR));
    __asm volatile("POP	{R0}");

    
    printf("dab:%08x, at:\r\n", faultAddress);
    while(1);

}

void volatile __attribute__((target("arm"))) __attribute__((naked)) arm_vector_irq()
{
    __asm volatile("sub  lr,lr,#4");
    __asm volatile("stmdb sp!, {r0-r12, lr}");
    do_irq();
    __asm volatile("ldmia sp!, {r0-r12, pc}^");
}


void volatile __attribute__((target("arm"))) __attribute__((naked)) arm_vector_fiq()
{
    print_uart0("fiq\r\n");
    while(1);
    
}