

#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include "llapi.h"
#include "app_config.h"

extern unsigned int _sbss;
extern unsigned int _ebss;

extern void (*__preinit_array_start[])(void) __attribute__((weak));
extern void (*__preinit_array_end[])(void) __attribute__((weak));
extern void (*__init_array_start[])(void) __attribute__((weak));
extern void (*__init_array_end[])(void) __attribute__((weak));

extern unsigned int __data_at_rom;
extern unsigned int __data_start;
extern unsigned int __data_end;

int main();
void llprint(const char *s); 
volatile void _init(uint32_t par0) __attribute__((naked)) __attribute__((target("arm"))) __attribute__((section(".init"))) ;
volatile void _init(uint32_t par0) 
{
 
    __asm volatile(INFO3);

    __asm volatile("mov r0,r0");
    __asm volatile("mov r0,r0");
    __asm volatile(".word StackSize");
    __asm volatile("mov r0,r0");
    __asm volatile("mov r0,r0");
    __asm volatile("mov r0,r0");
    __asm volatile("mov r0,r0");
    __asm volatile("mov r0,r0");
    

    for (uint8_t *i = (uint8_t *)&_sbss; i < (uint8_t *)&_ebss; i++) {
        *i = 0; // clear bss
    }
 
    uint8_t *pui32Src, *pui32Dest;
    pui32Src = (uint8_t *)&__data_at_rom;
    for (pui32Dest =  (uint8_t *)&__data_start; pui32Dest <  (uint8_t *)&__data_end;) {
        *pui32Dest++ = *pui32Src++;
    } 

    typedef void (*pfunc)();
    extern pfunc __ctors_start__[];
    extern pfunc __ctors_end__[];
    pfunc *p;
    for (p = __ctors_start__; p < __ctors_end__; p++) {
        (*p)();
    }
 
    exit(main());

    while(1);
}


extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);

void __libc_fini_array(void) {
    size_t count;
    size_t i;

    count = __fini_array_end - __fini_array_start;
    for (i = count; i > 0; i--)
        __fini_array_start[i - 1]();
}

void __sync_synchronize() {
}

