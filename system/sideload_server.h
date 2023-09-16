#pragma once
#include "FreeRTOS.h"
#include "task.h"

void sls_send_request(uint32_t pgoff, uint32_t fill_to_paddr, uint32_t vaddr, TaskHandle_t task);

void sls_init();



 