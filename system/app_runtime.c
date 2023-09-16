#include <fcntl.h>

#include "app_runtime.h"
#include "queue.h"
#include "../apps/llapi.h"

QueueHandle_t app_api_queue;

static void *app_mem_warp_alloc = NULL;
static uint32_t app_mem_warp_at = 0;
static uint32_t app_mem_size = 0;
static volatile bool app_running = false;
static StaticTask_t app_tcb;
static TaskHandle_t app_task_handle = NULL;
static int mmap = 0;

//static char mmaps[6];
uint32_t app_stack_sz = 0; 


bool app_is_running()
{
    return app_running;
}



uint32_t app_get_ram_size()
{
    return app_mem_size;
}




static void app_main_thread(void *par) {

    __asm volatile("mrs r1,cpsr_all");
    __asm volatile("bic r1,r1,#0x1f");
    __asm volatile("orr r1,r1,#0x10");
    __asm volatile("msr cpsr_all,r1");
    //memTestWrite(APP_RAM_MAP_ADDR, 360*1024,444);
    //memTestRead(APP_RAM_MAP_ADDR, 360*1024,0,444);

    __asm volatile("ldr r1,=%0" : : ""(APP_ROM_MAP_ADDR + 32) : );
    __asm volatile("bx r1");

    //vTaskDelete(NULL);
    while (1) { 
        vTaskDelay(pdMS_TO_TICKS(1000));
        ;
    }
}


mmap_info mf;

void app_stop()
{
    
    if(!app_running)
        return;
    if(!app_task_handle)
        return;

    vTaskSuspendAll();

    free((void *)mf.path);


    vTaskDelete(app_task_handle);

    void sls_unlock_all();
    sls_unlock_all();
    ll_set_app_mem_warp(0, 0);
    free(app_mem_warp_alloc);
    app_mem_warp_alloc = NULL;

    app_running = false;
    app_task_handle = NULL;

    //for(int i = 0; i < sizeof(mmaps); i++)
    //{
    //    if(mmaps[i])
    //        ll_mumap(mmaps[i]);
    //    mmaps[i] = 0;
    //}
    ll_mumap(mmap);

    for(uint32_t i = APP_RAM_MAP_ADDR; i < APP_RAM_MAP_ADDR + app_mem_size; i+=1024)
    {
        ll_mm_trim_vaddr(i & ~(1023));
    }

    xPortHeapTrimFreePage();
    app_mem_size = 0;
    xTaskResumeAll();
    vTaskDelay(pdMS_TO_TICKS(50));
}

static volatile uint32_t rdVal = 0;
static void app_rom_read(void *p)
{
    vTaskDelay(pdMS_TO_TICKS(100));
    rdVal = ((uint32_t *)p)[0];
    vTaskDelay(pdMS_TO_TICKS(100));
    vTaskDelete(NULL);
}

void app_start()
{
    if(app_running)
    {
        return;
    }
    if(mmap <= 0)
    {
        return;
    }

    bsp_diaplay_clean(0xFF); 

    uint32_t *stackSz = (uint32_t *)(APP_ROM_MAP_ADDR + 8);
    TaskHandle_t task_app_rom_read;
    xTaskCreate(app_rom_read, "app_rom_read", 32, stackSz, 2, &task_app_rom_read);
    vTaskDelay(pdMS_TO_TICKS(200));
    int i = 0;
    while (rdVal == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        i++;
        if(i > 10)
        {
            break;
        }
    }
    if(rdVal != 0xE1A00000)
    {
        app_stack_sz = rdVal;
        printf("SET Stack Size:%ld\r\n", app_stack_sz );
    }

    if(app_stack_sz == 0)
    {
        app_stack_sz = 400;
    }

    vTaskSuspendAll();
    uint32_t free_mem = getFreeMemSz() - 12*1024;
    app_mem_warp_alloc = malloc(free_mem);
    app_mem_warp_at = (uint32_t)app_mem_warp_alloc;
    while((app_mem_warp_at % 1024))
    {
        app_mem_warp_at++;
    }  
    app_mem_size = (free_mem/1024) - 1;
    ll_set_app_mem_warp(app_mem_warp_at, app_mem_size);
    app_mem_size = app_mem_size * 1024;

    printf("Allocate App Mem:%ld\r\n", app_mem_size);
     app_task_handle = xTaskCreateStatic(
         app_main_thread, 
          "app_t0", 
          app_stack_sz, 
          NULL, 1,
          (StackType_t *const)(APP_RAM_MAP_ADDR  + app_mem_size - app_stack_sz * 4 - 4) , &app_tcb); //APP_RAM_MAP_ADDR  + app_mem_size - 4 - app_stack_sz * 4
    //xTaskCreate(app_main_thread, "app_t0", 400, NULL, 1, &app_task_handle);
    
    xTaskResumeAll();
    app_running = true;
    gdb_attach_to_task(app_task_handle);
}


uint32_t app_get_exp_sz(char *path)
{
    fs_obj_t f = malloc(ll_fs_get_fobj_sz());
    int res = ll_fs_open(f, path, O_RDONLY);
    if(res)
    {
        free(f);
        return 0;
    }
    int sz = ll_fs_size(f);
    ll_fs_close(f);
    free(f);
    return sz;
}


void app_pre_start(char *path, bool sideload, uint32_t sideload_sz)
{
    
    app_stop();
    
    mf.map_to = APP_ROM_MAP_ADDR;
    mf.offset = 0;
    mf.writable = false;
    mf.writeback = false;
    if(sideload)
    {
        mf.size = sideload_sz;
        mf.path = calloc(1, 3);
        memcpy((void *)mf.path,(void *)"\03",1);
    }else{
        mf.path = calloc(1, 64);
        strncpy((char *)mf.path, path, 63);
        uint32_t expSz = app_get_exp_sz(path);
        //int32_t remainSz = expSz;
        printf("app rom sz:%d\r\n", expSz);
        mf.size = 0;
        //mmaps[0] = ll_mmap(&mf);
        mmap = ll_mmap(&mf);
        printf("A mmap:%d, off:%08lX,sz:%ld,  %s\r\n", mmap, mf.offset, mf.size,  mf.path);
        //for(int i = 0; i <= expSz / 1048576; i++)
        //{
        //    mf.map_to = APP_ROM_MAP_ADDR + i * 1048576;
        //    mf.offset = i * 1048576;
        //    mf.size = 1048576;
        //    remainSz -= mf.size;
        //    mmaps[i] = ll_mmap(&mf);
        //    printf("A mmap:%d, off:%08lX,sz:%ld,  %s\r\n", mmaps[i],mf.offset,mf.size,  mf.path);
        //    if((i == 1) || ((remainSz < 1048576) && (remainSz > 0)))
        //    {
        //        i++;
        //        mf.map_to = APP_ROM_MAP_ADDR + i * 1048576;
        //        mf.offset = i * 1048576;
        //        mf.size = 1048576;
        //        mf.size = remainSz;
        //        mmaps[i] = ll_mmap(&mf);
        //        printf("B mmap:%d, off:%08lX,sz:%ld,  %s\r\n", mmaps[i],mf.offset,mf.size,  mf.path);
        //        break;
        //    }
        //}

        
    }

    //mmap = ll_mmap(&mf);
    
}


struct task_delay_info_t
{
    TaskHandle_t task;
    uint32_t ms;
    uint32_t start_tick;
}task_delay_info_t;

static struct task_delay_info_t delay_task[10];

void app_api_task(void *p)
{ 
    app_api_info_t info;
    uint32_t ticks = 0;
    for(;;)
    {
        ticks++;

        for(int i = 0; i < 10; i++)
        {
            if(delay_task[i].task != NULL)
            {
                if((ticks - delay_task[i].start_tick) >= delay_task[i].ms)
                {
                    vTaskResume(delay_task[i].task);
                    delay_task[i].task = NULL;
                }
                break;
            }
        }


        if(xQueueReceive(app_api_queue, &info, pdMS_TO_TICKS(1)) == pdTRUE)
        {
            switch (info.code)
            {
                case LLAPI_APP_DELAY_MS:
                    vTaskSuspend(info.task);
                    for(int i = 0; i < 10; i++)
                    {
                        if(delay_task[i].task == NULL)
                        {
                            delay_task[i].task = info.task;
                            delay_task[i].ms = info.par0;
                            delay_task[i].start_tick = ticks;
                            break;
                        }
                    }
                break;

            default:
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

void app_api_init()
{ 
    app_api_queue = xQueueCreate(10, sizeof(app_api_info_t));
    xTaskCreate(app_api_task, "LLAPI Srv", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL);
}
