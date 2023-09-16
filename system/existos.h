#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "sys_mmap.h"
#include "hypercall.h"

#define APP_ROM_MAP_ADDR   (0x08000000)
#define APP_RAM_MAP_ADDR   (0x02000000 + (512*1024)) //0x02080000

#define INFO(...) do{printf(__VA_ARGS__);}while(0)

 int sys_query_key();
uint32_t getFreeMemSz() ;

