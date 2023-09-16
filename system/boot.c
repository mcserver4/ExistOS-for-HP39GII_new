
#include <stdint.h>



extern unsigned int __bss_start;
extern unsigned int __bss_end;

extern unsigned int __init_data;
extern unsigned int __data_start__;
extern unsigned int __data_end__;

extern unsigned int dma_data;
extern unsigned int __dma_data_start;
extern unsigned int __dma_data_end;


volatile void __init() __attribute__((naked)) __attribute__((target("arm"))) __attribute__((section(".init"))) ;
volatile void __init() 
{

    for (uint8_t *i = (uint8_t *)&__bss_start; i < (uint8_t *)&__bss_end; i++) {
        *i = 0; // clear bss
    }

    //for (char *i = (char *)&ucHeap; i < (char *)&ucHeapEnd; i++) {
    //    *i = 0; // clear heap
    //}

    uint8_t *pui32Src, *pui32Dest;
    pui32Src = (uint8_t *)&__init_data;
    for (pui32Dest =  (uint8_t *)&__data_start__; pui32Dest <  (uint8_t *)&__data_end__;) {
        *pui32Dest++ = *pui32Src++;
    }

    pui32Src = (uint8_t *)&dma_data;
    for (pui32Dest =  (uint8_t *)&__dma_data_start; pui32Dest <  (uint8_t *)&__dma_data_end;) {
        *pui32Dest++ = *pui32Src++;
    }

    int main();
    main();

}

