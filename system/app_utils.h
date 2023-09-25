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

typedef struct ExpUtilsConfig_t{
    char customFilePath[1024];//
}ExpUtilsConfig_t;

typedef struct Node
{
    char name[256];
    bool isDir;
    int size;
    struct Node *prev;
    struct Node *next;
} Node;

typedef struct Stack
{
    Node *head;
    unsigned int length;
} DIR_Element;

void app_create_folder();
void get_app_name(char *path,char *app_name);
void appUtilsInit();
