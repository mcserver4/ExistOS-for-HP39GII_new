
#include "sys_mmap.h"
#include "hypercall.h"
#include "sideload_server.h"
#include "gdbserver.h"
static QueueHandle_t QfaultInfo;
extern int fault_count;
static void sys_mmap_task(void *p0) {
    fault_info_t finfo;
    while (1) {
        while (xQueueReceive(QfaultInfo, &finfo, portMAX_DELAY) == pdTRUE) {
            vTaskSuspend(finfo.task);
            fault_count = 0;
            //if (finfo.ftype != 3) {
            //    if (ll_vaddr_vaild(finfo.faddr))
            //    {
            //         sls_send_request(ll_get_pending_pgfoff(), ll_get_pending_pgpaddr(), finfo.faddr, finfo.task);
            //    } else {
            //        goto err;
            //    }
            //} else {
            //    goto err;
            //}
//
            //continue;

        //err:
            printf("ftype:%d, faddr:%08lX, task:%s,%08lX\r\n", finfo.ftype, finfo.faddr, pcTaskGetName(finfo.task), (uint32_t)finfo.task);
            gdb_attach_to_task(finfo.task);
        }
    }
}

bool sys_mmap_push(TaskHandle_t task, uint32_t ftype, uint32_t faddr) {
    BaseType_t const sw;
    fault_info_t finfo;
    finfo.task = task;
    finfo.ftype = ftype;
    finfo.faddr = faddr;
    // xQueueSend(QfaultInfo, &finfo, portMAX_DELAY);
    xQueueSendToBackFromISR(QfaultInfo, &finfo, (BaseType_t *const)&sw);
    return sw;
}

void sys_mmap_init() {
    QfaultInfo = xQueueCreate(6, sizeof(fault_info_t));
    xTaskCreate(sys_mmap_task, "sysmmap", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
}
