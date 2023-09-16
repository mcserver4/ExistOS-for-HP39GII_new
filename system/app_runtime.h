#pragma once

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

typedef struct app_api_info_t
{
    TaskHandle_t task;
    uint32_t code;
    uint32_t par0;
    uint32_t par1;
    uint32_t par2;
    uint32_t par3;
}app_api_info_t;


void app_pre_start(char *path, bool sideload, uint32_t sideload_sz);

void app_api_init();

bool app_is_running();

uint32_t app_get_ram_size();

void app_stop();

void app_start();
