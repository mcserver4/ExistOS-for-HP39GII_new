
#include "existos.h"
#include <sys/fcntl.h>

#include "system/bsp/board.h"
#include "tusb.h"

#include "dhserver.h"
#include "dnserver.h"
#include "lwip/apps/fs.h"
#include "lwip/apps/httpd.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"

#include "FreeRTOS.h"
#include "app_runtime.h"
#include "sideload_server.h"
#include "task.h"

#define SLS_PORT 9002

static struct udp_pcb *pcb = NULL;

static ip_addr_t client_addr;
static u16_t client_port = 0;

static uint32_t pending_paddr = 0;
static uint32_t pending_vaddr = 0;
static uint32_t pending_pgoff = 0;
static TaskHandle_t pending_task;

static fs_obj_t sls_upload = NULL;

#define CMD_CMP(x) memcmp(p->payload, (x), (sizeof(x) - 1)) == 0

uint32_t hist1_lock_vaddr[3];
uint32_t hist2_lock_vaddr[3];
uint32_t *p_hist_lock_vaddr = hist1_lock_vaddr;

void sls_unlock_all() {
    for (int i = 0; i < sizeof(hist1_lock_vaddr) / 4; i++) {
        if (hist1_lock_vaddr[i]) {
            ll_vaddr_lock(hist1_lock_vaddr[i], 0);
            hist1_lock_vaddr[i] = 0;
        }
        if (hist2_lock_vaddr[i]) {
            ll_vaddr_lock(hist2_lock_vaddr[i], 0);
            hist2_lock_vaddr[i] = 0;
        }
    }
}

uint32_t last_vaddr = 0;
static void udp_recv_proc(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    char cmdbuf[64];
    uint32_t rec_pgoff;
    struct pbuf *rbuf = p;
    struct pbuf *out;
    uint32_t rec_len = 0;
    uint32_t pbufs = 0;

    client_addr = *addr;
    client_port = port;

    while (rbuf) {
        rec_len += rbuf->len;
        rbuf = rbuf->next;
        pbufs++;
    }
    if (pbufs != 1)
        goto recerr;

    out = pbuf_alloc(PBUF_TRANSPORT, 5, PBUF_POOL);
    if (!out) {
        printf("SLS_REC:NO MEM!\r\n");
        return;
    }

    if (CMD_CMP("PUT_")) {
        if (app_is_running()) {
            if (pending_paddr && pending_vaddr) {
                memcpy(cmdbuf, p->payload, 11);
                cmdbuf[11] = 0;
                sscanf(cmdbuf, "PUT_%07d", &rec_pgoff);

                // printf("rec:%d,%d, load to:%08lX, vaddr:%08lx\r\n", rec_pgoff, rec_len , pending_paddr, pending_vaddr  );
                if (rec_pgoff == pending_pgoff) {
                    if (ll_vaddr2paddr(pending_vaddr) == pending_paddr) {
                        if (last_vaddr != pending_vaddr) {
                            if (last_vaddr) {
                                *p_hist_lock_vaddr = last_vaddr;
                                p_hist_lock_vaddr++;

                                if (p_hist_lock_vaddr >= &hist1_lock_vaddr[sizeof(hist1_lock_vaddr) / 4]) {
                                    for (int i = 0; i < sizeof(hist1_lock_vaddr) / 4; i++) {
                                        if (hist1_lock_vaddr[i])
                                            ll_vaddr_lock(hist1_lock_vaddr[i], 0);
                                    }
                                    p_hist_lock_vaddr = &hist2_lock_vaddr[0];
                                } else if (p_hist_lock_vaddr >= &hist2_lock_vaddr[sizeof(hist2_lock_vaddr) / 4]) {
                                    for (int i = 0; i < sizeof(hist1_lock_vaddr) / 4; i++) {
                                        if (hist2_lock_vaddr[i])
                                            ll_vaddr_lock(hist2_lock_vaddr[i], 0);
                                    }
                                    p_hist_lock_vaddr = &hist1_lock_vaddr[0];
                                }
                            }
                            last_vaddr = pending_vaddr;
                        }

                        memcpy((void *)pending_paddr, &(((uint8_t *)p->payload)[11]), rec_len - 11);
                        mmu_invalidate_icache();
                        mmu_invalidate_dcache((void *)(pending_vaddr & ~(1024)), 1024);
                        vTaskResume(pending_task);

                        ((char *)out->payload)[0] = 'T';
                    } else {
                        printf("Mismatch: %08lX %08lX\r\n", pending_paddr, ll_vaddr2paddr(pending_vaddr));
                        ((char *)out->payload)[0] = 'F';
                    }
                } else {
                    printf("Mismatch PG: %08lX %08lX\r\n", pending_paddr, ll_vaddr2paddr(pending_vaddr));
                    ((char *)out->payload)[0] = 'F';
                }
            }
        }

        // printf("Task:%08lX\r\n", (uint32_t)pending_task);
        // printf("Resume Task:%s\r\n", pcTaskGetName(pending_task));
        //
        // printf("rest frame:%08lx\r\n", ((uint32_t *)pending_task)[1] );

    } else if (CMD_CMP("UPLOAD_")) {
        if (sls_upload) {
            ll_fs_close(sls_upload);
            free(sls_upload);
            sls_upload = NULL;
        }
        int ret = 0;
        memset(cmdbuf, 0, sizeof(cmdbuf));
        strncpy(cmdbuf, &((char *)p->payload)[7], 63 - 7);
        printf("upload:[%s]\r\n", cmdbuf);
        sls_upload = malloc(ll_fs_get_fobj_sz());
        if (sls_upload) {
            ret = ll_fs_open(sls_upload, cmdbuf, O_RDWR | O_CREAT | O_TRUNC);
            if (ret >= 0) {
                ((char *)out->payload)[0] = 'T';
            } else {
                ((char *)out->payload)[0] = 'F';
            }
        } else {
            ((char *)out->payload)[0] = 'F';
        }

    } else if (CMD_CMP("U_")) {
        if (sls_upload) {
            int ret = 0;
            memset(cmdbuf, 0, sizeof(cmdbuf));
            strncpy(cmdbuf, p->payload, sizeof("U_1234") - 1);
            uint32_t sz = 0;
            sscanf(cmdbuf, "U_%d", &sz);
            ret = ll_fs_write(sls_upload, &((uint8_t *)p->payload)[sizeof("U_1234") - 1], sz);
            if (ret >= 0) {
                ((char *)out->payload)[0] = 'T';
            } else {
                ((char *)out->payload)[0] = 'F';
            }
        } else {
            ((char *)out->payload)[0] = 'F';
        }
    } else if (CMD_CMP("UDONE")) {
        if (sls_upload) {
            printf("Upload Done\r\n");
            ll_fs_sync(sls_upload);
            ll_fs_close(sls_upload);
            free(sls_upload);
            sls_upload = NULL;
            ((char *)out->payload)[0] = 'T';
        } else {
            ((char *)out->payload)[0] = 'F';
        }
    } else if (CMD_CMP("EXEC_")) {
        int ret = 0;
        memset(cmdbuf, 0, sizeof(cmdbuf));
        strncpy(cmdbuf, &((char *)p->payload)[sizeof("EXEC_") - 1], sizeof(cmdbuf) - sizeof("EXEC_") - 1);
        printf("run:[%s]\r\n", cmdbuf);
        app_stop();
        app_pre_start(cmdbuf, false, 0);
        app_start();
        ((char *)out->payload)[0] = 'T';
    } else if (CMD_CMP("KILL")) {
        app_stop();
        ((char *)out->payload)[0] = 'T';
    } else if (CMD_CMP("SLS_START")) {
        last_vaddr = 0;
        pending_paddr = 0;
        pending_vaddr = 0;
        pending_pgoff = 0;
        pending_task = NULL;
        app_stop();
        app_pre_start(NULL, true, 1048576 * 64);
        app_start();
        ((char *)out->payload)[0] = 'T';
    }

    memcpy(&(((char *)out->payload)[1]), &rec_len, 4);

    udp_sendto(upcb, out, addr, port);

    pbuf_free(out);
    pbuf_free(p);

    return;

recerr:
    printf("SideLoad Srv Receive ERR\r\n");
    pbuf_free(p);
}

void sls_send_request(uint32_t pgoff, uint32_t fill_to_paddr, uint32_t vaddr, TaskHandle_t task) {
    if (!client_port)
        return;
    struct pbuf *out;
    out = pbuf_alloc(PBUF_TRANSPORT, 5, PBUF_RAM);
    if (!out) {
        INFO("SLS: sls_send_request NO MEM!\r\n");
        return;
    }
    pending_paddr = fill_to_paddr;
    pending_pgoff = pgoff;
    pending_vaddr = vaddr;
    pending_task = task;
    ((char *)out->payload)[0] = 'R';
    memcpy(&(((char *)out->payload)[1]), &pending_pgoff, 4);
    udp_sendto(pcb, out, &client_addr, client_port);

    pbuf_free(out);
}

void sls_init() {
    err_t err;
    pcb = udp_new();
    if (pcb == NULL) {
        INFO("SLS:init NO MEM!\r\n");
        return;
    }
    err = udp_bind(pcb, IP_ADDR_ANY, SLS_PORT);
    if (err != ERR_OK) {
        udp_remove(pcb);
        pcb = NULL;
        INFO("SLS:Bind port error!\r\n");
        return;
    }
    udp_recv(pcb, udp_recv_proc, NULL);
}
