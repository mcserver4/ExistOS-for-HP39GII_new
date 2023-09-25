
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <portmacro.h>
#include "board.h"

#include "sys_mmap.h"

#include "app_runtime.h"

#include "../apps/llapi.h"

extern volatile void *pxCurrentTCB;
extern volatile uint32_t ulCriticalNesting;
uint32_t
    __attribute__((naked))
    _get_sp() {
    asm volatile("mov r0,sp");
    asm volatile("bx lr");
}

#define SAVE_CONTEXT()                                                                                                \
    asm volatile("STMDB   SP, {R0-R14}^"); /*{NESTING_CNT, SPSR, R0-R15 <- } 16*4*/                                   \
    asm volatile("STR     LR, [SP]");                                                                                 \
    asm volatile("MRS     R0, SPSR");                                                                                 \
    asm volatile("STR     R0, [SP, #-64]"); /*{NESTING_CNT, SPSR <-, R0-R15 } 16*4*/                                  \
    asm volatile("LDR     R0, =ulCriticalNesting");                                                                   \
    asm volatile("LDR     R0, [R0]");                                                                                 \
    asm volatile("STR     R0, [SP, #-68]"); /*{NESTING_CNT <-, SPSR, R0-R15 } 16*4*/                                  \
    asm volatile("SUB     SP, SP, #68");                                                                              \
    uint32_t *context = (uint32_t *)_get_sp();                                                                        \
    uint32_t *pRegFram = (uint32_t *)((uint32_t *)pxCurrentTCB)[1];                                                   \
    memcpy(pRegFram, context, 18 * 4);                                                                                \
    ((uint32_t *)pxCurrentTCB)[1] = ((uint32_t *)pxCurrentTCB)[1] + 18 * 4; /*Set The Registers frame stack pointer*/ \
    ((uint32_t *)pxCurrentTCB)[0] = context[13 + 2];                        /*Set the task stack pointer*/


//uint32_t swi_stack[400];
uint32_t swi_code;
uint32_t svc_sys_sp;
uint32_t *svc_sp;
uint32_t in_svc = 0;
 volatile uint32_t *fcon_svc;

 volatile uint32_t *svc_context_ptr;

 
void __attribute__((naked)) __attribute__((target("arm"))) load_sp(uint32_t sp)
{
    __asm volatile("mov sp,r0");
    __asm volatile("bx lr");
}

uint32_t __attribute__((naked)) __attribute__((target("arm"))) get_sp()
{
    __asm volatile("mov r0,sp");
    __asm volatile("bx lr");
}

void __attribute__((naked)) __attribute__((target("arm"))) load_lr(uint32_t lr)
{
    __asm volatile("mov r1,lr");
    __asm volatile("mov lr,r0");
    __asm volatile("bx r1");
}

void __attribute__((naked)) __attribute__((target("arm"))) sys_svc()
{
    
    //__asm volatile("ldr r2,=svc_sp");
    //__asm volatile("str r8,[r2]");
  //  __asm volatile("push {r4-r12,lr}");

    __asm volatile("ldr r1,=swi_code");
    __asm volatile("str r0,[r1]");

    __asm volatile("ldr r2,=fcon_svc");
    __asm volatile("str r4,[r2]");
  
    __asm volatile("ldr r2,=svc_context_ptr");
    __asm volatile("str r5,[r2]");

    //__asm volatile("ldr r2,=svc_sys_sp");
    //__asm volatile("str r7,[r2]");
    
    fcon_svc-=17;
    uint32_t *pRegFram = (uint32_t *)((uint32_t *)pxCurrentTCB)[1];                                                   
    memcpy(&pRegFram[1], (void *)fcon_svc, 17 * 4);    
    pRegFram[0] = ulCriticalNesting;                                                                             
    ((uint32_t *)pxCurrentTCB)[1] = ((uint32_t *)pxCurrentTCB)[1] + 18 * 4; /*Set The Registers frame stack pointer*/ 
    ((uint32_t *)pxCurrentTCB)[0] = fcon_svc[13 + 1];                        /*Set the task stack pointer*/

   //  for(int i = 0; i  < 18; i++)
   //  {
   //      printf("DAB C:%d :%08lX\r\n", i, fcon_dab[i]);
   //  }
     *svc_context_ptr -= 17*4;

     
    in_svc++;
    //printf("sp A:%08lX, %08lX\r\n", fcon_svc[13 + 1], get_sp());
    if(in_svc > 1)
    {
        printf("! reent sys svc!:%d\r\n", in_svc);
        //load_sp(fcon_svc[13 + 1]);
        //load_lr(fcon_svc[14 + 1]);
    }
    //printf("sp B:%08lX, %08lX\r\n", fcon_svc[13 + 1], get_sp());
   // __asm volatile("ldr r2,=svc_sys_sp");
   // __asm volatile("str r7,[r2]");

//    __asm volatile("ldmia sp!, {r0-r12, lr}"); 
//    SAVE_CONTEXT();
//    __asm volatile("add sp,sp,#68");

    //__asm volatile("mrs r1,cpsr_all");
    //__asm volatile("bic r1,r1,#0x1f");
    //__asm volatile("orr r1,r1,#0x13");
    //__asm volatile("msr cpsr_all,r1");

    //__asm volatile("ldr r0,=swi_stack");
    //__asm volatile("add sp,r0,#1600");
    //__asm volatile("sub sp,sp,#4");

    extern QueueHandle_t app_api_queue;

    switch (swi_code)
    {
    case LLAPI_APP_DELAY_MS: 
        {
            app_api_info_t info;
            uint32_t ShouldYield = 0;
            info.task = (TaskHandle_t)pxCurrentTCB;
            info.code = swi_code;
            info.par0 = pRegFram[0 + 2];
            xQueueSendFromISR(app_api_queue, &info, &ShouldYield);
            if(ShouldYield)
            {
                vTaskSwitchContext(); 
            }
        }
        //app_thread_delay_ms(pRegFram[0 + 2]);
        break;
    case LLAPI_APP_STDOUT_PUTC:
        putchar(pRegFram[0 + 2]);
        break;
    case LLAPI_APP_GET_RAM_SIZE:
        pRegFram[0 + 2] = app_get_ram_size();
        break;
    case LLAPI_APP_GET_TICK_MS:
        pRegFram[0 + 2] = bsp_time_get_ms();
        break;
    case LLAPI_APP_GET_TICK_US:
        pRegFram[0 + 2] = bsp_time_get_us();
        break;
    case LLAPI_APP_RTC_GET_S:
        pRegFram[0 + 2] = bsp_rtc_get_s();
        break;
    case LLAPI_APP_RTC_SET_S:
        bsp_rtc_set_s(pRegFram[0 + 2]);
        break;
    case LLAPI_APP_DISP_PUT_P:
        //printf("SET: %ld,%ld,%ld\r\n",  pRegFram[0 + 2], pRegFram[1 + 2], pRegFram[2 + 2] );
        bsp_display_set_point(pRegFram[0 + 2], pRegFram[1 + 2], pRegFram[2 + 2]);
        break;
    case LLAPI_APP_DISP_GET_P:
        pRegFram[0 + 2] =  bsp_display_get_point(pRegFram[0 + 2], pRegFram[1 + 2]);
        break;
    case LLAPI_APP_DISP_PUT_HLINE:
        bsp_diaplay_put_hline(pRegFram[0 + 2], (void *)pRegFram[1 + 2]);
        break;
    case LLAPI_APP_QUERY_KEY:
        pRegFram[0 + 2] = sys_query_key();
        break;
/*    case LLAPI_APP_DISP_PUT_BUFFER:
        bsp_display_put_buffer((uint8_t *)pRegFram[0 + 2],pRegFram[1 + 2],pRegFram[2 + 2],(Coords_t*)pRegFram[3 + 2]);
        break;*/
    
    case 0x55:
        vTaskSwitchContext(); 
        break;
    case 0x77: 
        __asm volatile("swi #0x66");
        break;
    default:
        
        //__asm volatile("add sp,sp,#32");
        printf("SWI:%08lX\r\n", swi_code);
        break;
    }



    //  __asm volatile("ldr r2,=svc_sys_sp");
    //  __asm volatile("ldr sp,[r2]");
        //__asm volatile("ldr r2,=svc_sys_sp");
        //__asm volatile("ldr sp,[r2]");
    
    //__asm volatile("ldr r2,=svc_sp");
    //__asm volatile("ldr r2,[r2]");
    //__asm volatile("str sp,[r2]");
//
  // __asm volatile("mov r7,sp");
  // __asm volatile("mov r8,lr");
  //    __asm volatile("mrs r1,cpsr_all");
  //    __asm volatile("bic r1,r1,#0x1f");
  //    __asm volatile("orr r1,r1,#0x13");
  //    __asm volatile("msr cpsr_all,r1");
  //// __asm volatile("mov sp,r7");
  // __asm volatile("mov lr,r8");
    
    in_svc--;
    //ll_exit_svc();

    __asm volatile("mrs r1,cpsr_all");
    __asm volatile("bic r1,r1,#0x1f");
    __asm volatile("orr r1,r1,#0x13");
    __asm volatile("msr cpsr_all,r1");
    portRESTORE_CONTEXT();

}


//uint32_t irq_stack[400];
void __attribute__((naked)) __attribute__((target("arm"))) sys_irq()
{
    __asm volatile("ldmia sp!, {r0-r12, lr}");
    SAVE_CONTEXT();
    __asm volatile("add sp,sp,#68");

    //__asm volatile("mrs r1,cpsr_all");
    //__asm volatile("bic r1,r1,#0x1f");
    //__asm volatile("orr r1,r1,#0x13");
    //__asm volatile("msr cpsr_all,r1");
//
    //__asm volatile("ldr r0,=irq_stack");
    //__asm volatile("add sp,r0,#1600");
    //__asm volatile("sub sp,sp,#4");


    do_irq();

    portRESTORE_CONTEXT();
}

 
 
 volatile uint32_t faddr_dab;
 volatile uint32_t ftype_dab;
 volatile uint32_t *fcon_dab;
 volatile uint32_t *dab_context_ptr;
 
 
 volatile uint32_t faddr_pab;
 volatile uint32_t ftype_pab;
 volatile uint32_t *fcon_pab;
 volatile uint32_t *pab_context_ptr;
 
 #define FTYPE_PAB   1
 #define FTYPE_DAB   0
//
//void   __attribute__((target("arm")))  sys_do_dab(volatile uint32_t *fcon, volatile uint32_t ftype, volatile uint32_t faddr)
//{ 
//    //uint32_t *curStack = (uint32_t *)fcon[1+13];
//  //  printf("INABT:%ld,%08lX, %08lX\r\n", ftype ,faddr,(uint32_t)fcon);
//    //printf("Cur Stack:%08lX\r\n", (uint32_t)curStack);
//    if(ftype == FTYPE_PAB)
//    {
//        *pab_context_ptr = *pab_context_ptr - 17*4;
//      //  printf("pab_context_ptr:%08lX\r\n", *pab_context_ptr);
//    }else{
//        *dab_context_ptr = *dab_context_ptr - 17*4;
//       // printf("dab_context_ptr:%08lX\r\n", *dab_context_ptr);
//    }
//    for(int i = 0; i < 17; i++)
//    {
//      //  printf("CONTEXT:%d, %08lX\r\n",i, fcon[i]);
//    }
//
//   // printf("Cur TCB Stack:%08lX\r\n", ((uint32_t *)pxCurrentTCB)[1] );
//    
//    uint32_t *context = (uint32_t *)fcon;                                                                         
//    uint32_t *pRegFram = (uint32_t *)((uint32_t *)pxCurrentTCB)[1];       
//    pRegFram[0] = ulCriticalNesting;                                             
//    memcpy(&pRegFram[1], context, 17 * 4);                                                                                 
//    ((uint32_t *)pxCurrentTCB)[1] = ((uint32_t *)pxCurrentTCB)[1] + 18 * 4; /*Set The Registers frame stack pointer*/  
//    ((uint32_t *)pxCurrentTCB)[0] = context[13 + 2];                        /*Set the task stack pointer*/
//    
// 
//
//    //__asm volatile("swi #0");
//    //__asm volatile("swi #0");
//    //vTaskSwitchContext(); 
//    //printf("Task: %s\r\n", pcTaskGetName((TaskHandle_t)pxCurrentTCB) );
//    if(sys_mmap_push((TaskHandle_t)pxCurrentTCB, ftype, faddr))
//    {
//        vTaskSwitchContext(); 
//    }
//    //vTaskSuspend(NULL);
//    //while(1);
//    //sys_mmap_push((TaskHandle_t)pxCurrentTCB, ftyp, fadr);
//}

void __attribute__((target("arm"))) do_abt()
{

}


int fault_count = 0;
//uint32_t dab_sys_sp = 0;
uint32_t *dab_sp;
void __attribute__((naked)) __attribute__((target("arm"))) sys_dab()
{
         __asm volatile("ldr r3,=ftype_dab");
         __asm volatile("ldr r2,=faddr_dab");
         __asm volatile("str r0,[r3]");
         __asm volatile("str r1,[r2]");
  
         __asm volatile("ldr r2,=fcon_dab");
         __asm volatile("str r4,[r2]");
  
         __asm volatile("ldr r2,=dab_context_ptr");
         __asm volatile("str r5,[r2]");
  ///
         //__asm volatile("ldr r2,=dab_sys_sp");
         //__asm volatile("str r7,[r2]");
  ///
   ///   __asm volatile("ldr r2,=dab_sp");
   ///   __asm volatile("str r8,[r2]");


    //__asm volatile("ldmia sp!, {r0-r12, lr}");
    //SAVE_CONTEXT();
    //__asm volatile("add sp,sp,#68");



 
    fcon_dab-=17;


    uint32_t *pRegFram = (uint32_t *)((uint32_t *)pxCurrentTCB)[1];                                                   
    memcpy(&pRegFram[1], (void *)fcon_dab, 17 * 4);    
    pRegFram[0] = ulCriticalNesting;                                                                             
    ((uint32_t *)pxCurrentTCB)[1] = ((uint32_t *)pxCurrentTCB)[1] + 18 * 4; /*Set The Registers frame stack pointer*/ 
    ((uint32_t *)pxCurrentTCB)[0] = fcon_dab[13 + 1];                        /*Set the task stack pointer*/

   //  for(int i = 0; i  < 18; i++)
   //  {
   //      printf("DAB C:%d :%08lX\r\n", i, fcon_dab[i]);
   //  }


     *dab_context_ptr -= 17*4;
    fault_count++;
    if(fault_count > 10)
    {
        bsp_reset();
    }
    //
     if(sys_mmap_push((TaskHandle_t)pxCurrentTCB, ftype_dab, faddr_dab))
     {
         
        vTaskSwitchContext(); 
     }

    //__asm volatile("ldmia sp!, {r0-r12, lr}");
    //SAVE_CONTEXT();
    
    //__asm volatile("add sp,sp,#68");

    //printf("dab sp:%08lx\r\n", (uint32_t)context);
    

  //  printf("dab sp:%08lx\r\n", (uint32_t)_get_sp());

    //__asm volatile("mrs r1,cpsr_all");
    //__asm volatile("bic r1,r1,#0x1f");
    //__asm volatile("orr r1,r1,#0x13");
    //__asm volatile("msr cpsr_all,r1");
//
   // __asm volatile("ldr r0,=dab_stack");
   // __asm volatile("add sp,r0,#1600");
   // __asm volatile("sub sp,sp,#4");
 


   // *dab_context_ptr -= 17*4;
    //__asm volatile("add lr,lr,#4");
    //portSAVE_CONTEXT();
  //  printf("dab_sys_sp:%08lx\r\n",dab_sys_sp);
  //  printf("dab_sp:%08lx\r\n",*dab_sp);

 //   fcon_dab -= 17;
 //   //printf("dab save frame:%08lx\r\n", ((uint32_t *)pxCurrentTCB)[1] );
 //   sys_do_dab(fcon_dab, ftype_dab,faddr_dab);
 //
 //   //__asm volatile("pop {lr}");
 //
 //     //    __asm volatile("mrs r1,cpsr_all");
 //     //    __asm volatile("bic r1,r1,#0x1f");
 //     //    __asm volatile("orr r1,r1,#0x1b");
 //     //    __asm volatile("msr cpsr_all,r1");
 //   //
//
//
       //__asm volatile("ldr r2,=dab_sys_sp");
       //__asm volatile("ldr sp,[r2]");
//
 //   __asm volatile("mrs r1,cpsr_all");
 //   __asm volatile("bic r1,r1,#0x1f");
 //   __asm volatile("orr r1,r1,#0x13");
 //   __asm volatile("msr cpsr_all,r1");
//
        __asm volatile("mrs r1,cpsr_all");
        __asm volatile("bic r1,r1,#0x1f");
        __asm volatile("orr r1,r1,#0x13");
        __asm volatile("msr cpsr_all,r1");
    portRESTORE_CONTEXT();
}

uint32_t pab_sys_sp;
uint32_t *pab_sp;
void __attribute__((naked)) __attribute__((target("arm"))) sys_pab()
{
    __asm volatile("ldr r3,=ftype_pab");
    __asm volatile("ldr r2,=faddr_pab");
    __asm volatile("str r0,[r3]");
    __asm volatile("str r1,[r2]");

    __asm volatile("ldr r2,=fcon_pab");
    __asm volatile("str r4,[r2]");

    __asm volatile("ldr r2,=pab_context_ptr");
    __asm volatile("str r5,[r2]");

    __asm volatile("ldr r2,=pab_sys_sp");
    __asm volatile("str r7,[r2]");

    //__asm volatile("ldmia sp!, {r0-r12, lr}");
    //SAVE_CONTEXT();
    //__asm volatile("add sp,sp,#68");

 
   // print_uart0("SYS PAB\r\n");
 
    fcon_pab-=17;


    uint32_t *pRegFram = (uint32_t *)((uint32_t *)pxCurrentTCB)[1];                                                   
    memcpy(&pRegFram[1], (void *)fcon_pab, 17 * 4);    
    pRegFram[0] = ulCriticalNesting;                                                                             
    ((uint32_t *)pxCurrentTCB)[1] = ((uint32_t *)pxCurrentTCB)[1] + 18 * 4; /*Set The Registers frame stack pointer*/ 
    ((uint32_t *)pxCurrentTCB)[0] = fcon_dab[13 + 1];                        /*Set the task stack pointer*/

    // for(int i = 0; i  < 18; i++)
    // {
    //     printf("PAB C:%d :%08lX\r\n", i, fcon_pab[i]);
    // }


     *pab_context_ptr -= 17*4;


    if(sys_mmap_push((TaskHandle_t)pxCurrentTCB, ftype_pab, faddr_pab))
    {
       
     vTaskSwitchContext(); 
    }


    fault_count++;
    if(fault_count > 10)
    {
        bsp_reset();
    }
    //vTaskSuspend(NULL);

    


   //__asm volatile("ldr r2,=fcon_pab");
   //__asm volatile("str r4,[r2]");
   //  __asm volatile("ldr r2,=pab_context_ptr");
   //  __asm volatile("str r5,[r2]");

   //  __asm volatile("ldr r2,=pab_sys_sp");
   //  __asm volatile("str r7,[r2]");

   //  __asm volatile("ldr r2,=pab_sp");
   //  __asm volatile("str r8,[r2]");

   //  fcon_pab -= 17;
   //  //__asm volatile("ldmia sp!, {r0-r12, lr}");
   //    //SAVE_CONTEXT();
   //    //__asm volatile("add sp,sp,#68");

   //

   // // *pab_context_ptr -= 17*4;
   //  //printf("pab sp:%08lx\r\n", (uint32_t)_get_sp());

   //  //printf("pab sp:%08lx\r\n", (uint32_t)context);

   // // __asm volatile("mrs r1,cpsr_all");
   // // __asm volatile("bic r1,r1,#0x1f");
   // // __asm volatile("orr r1,r1,#0x13");
   // // __asm volatile("msr cpsr_all,r1");
   // //
   ////  __asm volatile("ldr r0,=pab_stack");
   ////  __asm volatile("add sp,r0,#1600");
   ////  __asm volatile("sub sp,sp,#4");
   //
   //  
   //  //__asm volatile("push {lr}");

   // // for(int i = 0; i <= 17; i++)
   // // {
   // //     printf("cont:%d,%08lX\r\n", i,context[i]);
   // // }

   //  //__asm volatile("add lr,lr,#4");
   //  //portSAVE_CONTEXT();
   //  //printf("pab save frame:%08lx\r\n", ((uint32_t *)pxCurrentTCB)[1] );
   //  
   //  //printf("pab_sys_sp:%08lx\r\n",pab_sys_sp);
   //  //printf("pab_sp:%08lx\r\n",*pab_sp);
   //  sys_do_dab(fcon_pab, ftype_pab,faddr_pab); 
   //

   //
   //    //    __asm volatile("mrs r1,cpsr_all");
   //    //    __asm volatile("bic r1,r1,#0x1f");
   //    //    __asm volatile("orr r1,r1,#0x1b");
   //    //    __asm volatile("msr cpsr_all,r1");
   //  //

   //  //__asm volatile("pop {lr}");

   //  
    //  __asm volatile("ldr r2,=pab_sys_sp");
    //  __asm volatile("ldr sp,[r2]");

   //   __asm volatile("mrs r1,cpsr_all");
   //   __asm volatile("bic r1,r1,#0x1f");
   //   __asm volatile("orr r1,r1,#0x13");
   //   __asm volatile("msr cpsr_all,r1");


        __asm volatile("mrs r1,cpsr_all");
        __asm volatile("bic r1,r1,#0x1f");
        __asm volatile("orr r1,r1,#0x13");
        __asm volatile("msr cpsr_all,r1");
    portRESTORE_CONTEXT();
}

