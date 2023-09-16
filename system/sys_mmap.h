#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include "existos.h"
#include "queue.h"

typedef struct fault_info_t
{
    TaskHandle_t task;
    uint32_t ftype;
    uint32_t faddr;
}fault_info_t;



bool sys_mmap_push(TaskHandle_t task, uint32_t ftype, uint32_t faddr);

void sys_mmap_init();
