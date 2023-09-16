#include "FreeRTOS.h"
#include "existos.h"
#include "gdbserver.h"
#include "hypercall.h"
#include "sys_mmap.h"
#include "system/bsp/board.h"
#include "task.h"
#include "tusb.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

#include <malloc.h>

#include "xformatc.h"
#include "app_runtime.h"
#include "sideload_server.h"

int coremain();

extern bool sys_state_print;

uint32_t getFreeMemSz() {
    struct mallinfo minfo;
    minfo = mallinfo();
    return minfo.fordblks + xPortGetFreeHeapSize();
}

void SysMonitor(void *t) {
    uint64_t tick = 0;

    struct mallinfo minfo;
    char *pcWriteBuffer = calloc(1, 2048);

    printf("Sys start.\r\n ");

    fs_dir_obj_t dir = malloc(ll_fs_get_dirobj_sz());
    printf("----lsdir------\r\n");
    int ret = ll_fs_dir_open(dir, "/");
    while (ll_fs_dir_read(dir) > 0) {
        printf("%s   \t%d   \t%d\r\n", ll_fs_dir_cur_item_name(dir), ll_fs_dir_cur_item_type(dir), ll_fs_dir_cur_item_size(dir));
    }
    printf("---------------\r\n");
    ll_fs_dir_close(dir);
    free(dir);

    while (1) {
#if configUSE_TRACE_FACILITY
        if (sys_state_print) {
            vTaskList(pcWriteBuffer);
            printf("=============SYSTEM STATUS=================\r\n");
            printf("Task Name   Task Status   Priority   Stack   ID\n");
            printf("%s\n", pcWriteBuffer);
            printf("Task Name   Running Count         CPU %%\n");
            vTaskGetRunTimeStats(pcWriteBuffer);
            printf("%s\n", pcWriteBuffer);
            printf("Status:  X-Running  R-Ready  B-Block  S-Suspend  D-Delete\n");
            minfo = mallinfo();
            printf("Allocated:%d Bytes\r\n", minfo.uordblks);
            printf("Free Fordblks:%ld Bytes\r\n", minfo.fordblks);
            printf("Free Heap Area:%ld Bytes\r\n", xPortGetFreeHeapSize());
            printf("Allocated Pages:%ld \r\n", xPortGetAllocatedPages());
            printf("Free Pages:%ld \r\n", xPortGetFreePages());
            printf("Free Total:%ld Bytes\r\n", minfo.fordblks + xPortGetFreeHeapSize());
            ///if(!app_is_running())
            {
                printf("memory trim:%d,%d\r\n", xPortHeapTrimFreePage(), malloc_trim(0));
            }
            printf("Tick:%lld\r\n", tick += (uint32_t)t);
            vTaskClearUsage();
        }
#endif
        vTaskDelay(pdMS_TO_TICKS(1000 * (uint32_t)t));
    }
}

void usbTask() {
    int usb_main(void);
    usb_main();
}

static volatile char key_tab1[KEY_COLS][KEY_ROWS];
static volatile char key_tab2[KEY_COLS][KEY_ROWS];
uint8_t key_rb[128];
int key_rb_wp = 0;
int key_rb_rp = 0;

int sys_query_key()
{
    int ret = 0;
    if(key_rb_rp == key_rb_wp)
    {
        return -1;
    }
    ret = key_rb[key_rb_rp];
    key_rb_rp++;
    if(key_rb_rp > sizeof(key_rb)-1)
    {
        key_rb_rp = 0;
    }
    return ret;
}

void sys() {
    uint32_t ticks = 0;
    vTaskDelay(pdMS_TO_TICKS(100));
    bsp_keyboard_init();
    bsp_display_init();
    gdb_server_init();
    sls_init();
    app_api_init();

    bsp_diaplay_clean(0xFF); 

    //uint8_t *p = calloc(1,370*1024);
    //volatile uint8_t *q = (volatile uint8_t *)p+2048;

   // register uint32_t tk asm("r0");
    while (1) {

  
        
         // __asm volatile("ldr r0,=1000":::"r0");
         // __asm volatile("swi #0xD701"); 

         //
         // __asm volatile("swi #0xD704": "=r"(tk));
         // printf("tk:%ld\r\n", tk);
        //*q = 0x32;
        //ll_mm_trim_vaddr((uint32_t)q);


        bsp_display_flush();

        bsp_scan_key(key_tab1);
        for (int y = 0; y < KEY_ROWS; y++) {
            for (int x = 0; x < KEY_COLS; x++) {
                int d = key_tab2[x][y] - key_tab1[x][y];
                if(d > 0)
                {
                    //printf("X:%d, Y:%d, s:%d\r\n",x,y,d);
                    key_rb[key_rb_wp++] = ((y << 3) + x) | KEY_S_RELEASE;
                }else if(d < 0){
                    //printf("X:%d, Y:%d, s:%d\r\n",x,y,d);
                    key_rb[key_rb_wp++] = ((y << 3) + x);
                }
                if(key_rb_wp > sizeof(key_rb) - 1)
                {
                    key_rb_wp = 0;
                }
            }
        }
         
        bsp_scan_key(key_tab2);
        vTaskDelay(pdMS_TO_TICKS(15));

        ticks++;
    }
}

//uint32_t dab_stack[400];
//uint32_t pab_stack[400];
/// uint32_t svc_stack[600];

int main() {

    ll_set_irq_vector((uint32_t)sys_irq);
    ll_set_svc_vector((uint32_t)sys_svc);
    ll_set_dab_vector((uint32_t)sys_dab);
    ll_set_pab_vector((uint32_t)sys_pab);

    //ll_set_dab_stack((uint32_t)&dab_stack[sizeof(dab_stack) - 2]);
    //ll_set_pab_stack((uint32_t)&pab_stack[sizeof(pab_stack) - 2]);
    /// ll_set_svc_stack((uint32_t)&svc_stack[sizeof(svc_stack) - 2]);

    switch_mode(SVC_MODE);

    print_uart0("PR SRART 1\r\n");
    // coremain();

    // xTaskCreate(TaskTest1, "test1", configMINIMAL_STACK_SIZE, (void *)1000, 2, NULL);

    sys_mmap_init();

    xTaskCreate(SysMonitor, "SysMonitor", configMINIMAL_STACK_SIZE, (void *)60, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(sys, "sys", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(usbTask, "TinyUsb", 370, NULL, configMAX_PRIORITIES - 2, NULL); 

    vTaskStartScheduler();
    while (1)
        ;
}

StackType_t xIdleTaskStack[configMINIMAL_STACK_SIZE];
StaticTask_t xIdleTaskTCB;
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize) {
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = xIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}


static uint32_t rt_tick;
uint32_t rt_tick_get()
{
    uint32_t val = bsp_time_get_us() - rt_tick;  
    return val;
}

void rt_tick_reset()
{
    rt_tick = bsp_time_get_us();
}
