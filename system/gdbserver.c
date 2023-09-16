

#include "board.h"
#include "tusb.h"

#include "dhserver.h"
#include "dnserver.h"
#include "lwip/init.h"
#include "lwip/tcp.h"
#include "lwip/timeouts.h"

#include "hypercall.h"

#include "FreeRTOS.h"
#include "task.h"
#include "gdbserver.h"

#include "app_runtime.h"

#define GDB_PORT 1234

#define GDB_PACK_Query "qSupported"
#define GDB_PACK_MRE "vMustReplyEmpty"
#define GDB_PACK_NACK "QStartNoAckMode"

#define GDB_REP_SUPPORT "PacketSize=1024;swbreak+;qXfer:features:read+;QStartNoAckMode+"
#define GDB_REP_OK "OK"
#define GDB_XML_TARGET "l<?xml version=\"1.0\"?><!DOCTYPE target SYSTEM \"gdb-target.dtd\"><target><architecture>arm</architecture><xi:include href=\"arm-core.xml\"/></target>"

#define GDB_XML_ARM_CORE "l<?xml version=\"1.0\"?><!DOCTYPE feature SYSTEM \"gdb-target.dtd\"><feature name=\"org.gnu.gdb.arm.core\"><reg name=\"r0\" bitsize=\"32\"/><reg name=\"r1\" bitsize=\"32\"/><reg name=\"r2\" bitsize=\"32\"/><reg name=\"r3\" bitsize=\"32\"/><reg name=\"r4\" bitsize=\"32\"/><reg name=\"r5\" bitsize=\"32\"/><reg name=\"r6\" bitsize=\"32\"/><reg name=\"r7\" bitsize=\"32\"/><reg name=\"r8\" bitsize=\"32\"/><reg name=\"r9\" bitsize=\"32\"/><reg name=\"r10\" bitsize=\"32\"/><reg name=\"r11\" bitsize=\"32\"/><reg name=\"r12\" bitsize=\"32\"/><reg name=\"sp\" bitsize=\"32\" type=\"data_ptr\"/><reg name=\"lr\" bitsize=\"32\"/><reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/><reg name=\"cpsr\" bitsize=\"32\" regnum=\"25\"/></feature>"

static char *streamBuf;
static char *sendBuf;
static int p = 0;
static int startRec = 0;

static TaskHandle_t AttachedTask = NULL;

static void sendPack(struct tcp_pcb *pcb, char *s) {
    int ret;
    uint8_t checkSum = 0;
    for (int i = 0; i < strlen(s); i++) {
        checkSum += s[i];
    }
    ret = snprintf(sendBuf, 2048, "+$%s#%02X", s, checkSum);
    printf("send:%s\r\n", sendBuf);
    if (ret > 0) {
        tcp_write(pcb, sendBuf, ret, TCP_WRITE_FLAG_COPY);
    }
}

#define TST_CMD(x) if (!memcmp(streamBuf, x, sizeof(x) - 1))

static void parse(struct tcp_pcb *pcb) {
    printf("pack:%s\r\n", streamBuf);
    TST_CMD(GDB_PACK_Query) {
        sendPack(pcb, GDB_REP_SUPPORT);
        return;
    }
    else TST_CMD(GDB_PACK_MRE) {
        sendPack(pcb, "");
        return;
    }
    else TST_CMD(GDB_PACK_NACK) {
        sendPack(pcb, GDB_REP_OK);
        return;
    }
    else TST_CMD("Hg0") {
        sendPack(pcb, "");
        return;
    }
    else TST_CMD("qTStatus") {
        sendPack(pcb, "");
        return;
    }
    else TST_CMD("qfThreadInfo") {
        sendPack(pcb, "");
        return;
    }
    else TST_CMD("qL1200000000000000000") {
        sendPack(pcb, "");
        return;
    }
    else TST_CMD("Hc-1") {
        sendPack(pcb, "");
        return;
    }
    else TST_CMD("qC") {
        sendPack(pcb, "");
        return;
    }
    else TST_CMD("Hg0") {
        sendPack(pcb, "");
        return;
    }
    else TST_CMD("qOffsets") {
        sendPack(pcb, "");
        return;
    }
    else TST_CMD("qAttached") {
        sendPack(pcb, "");
        return;
    }
    else TST_CMD("qSymbol") {
        sendPack(pcb, "");
        return;
    }
    else TST_CMD("?") {
        sendPack(pcb, "S05"); // POSIX signal SIGTRAP
        return;
    } 
    else TST_CMD("qXfer:features:read:target.xml") {
        sendPack(pcb, GDB_XML_TARGET);
        return;
    }
    else TST_CMD("qXfer:features:read:arm-core.xml") {
        sendPack(pcb, GDB_XML_ARM_CORE);
        return;
    }
    else TST_CMD("vCont?") {
        sendPack(pcb, "vCont;c;t"); //support cont,stop,
        return;
    }
    else TST_CMD("vCtrlC") {
        sendPack(pcb, "OK");
        return;
    }    
    else TST_CMD("c") { //‘C sig[;addr]’
        sendPack(pcb, "OK");
        return;
    }
    else TST_CMD("s") { //‘step’
        sendPack(pcb, "S05");
        return;
    }
    else TST_CMD("m") {
        //char text[20];
        char *text = calloc(1,1026); 
        uint32_t addr, width;
        sscanf(streamBuf, "m%x,%d", &addr, &width);
        printf("mmrd:%08lX,%d\r\n", addr, width);
        if (ll_vaddr_vaild(addr) || ((addr >= APP_RAM_MAP_ADDR) && (addr < APP_RAM_MAP_ADDR + app_get_ram_size()))) {
            uint8_t *p = (uint8_t *)addr;
            for(int i = 0; (i < 1026) && (i < width); i++ )
            {
                sprintf(&text[i*2], "%02lX", *p);
                p++;
            }
            sendPack(pcb, text);
        } else {

            sendPack(pcb, "");
        }
        free(text);
        return;
    }
    else TST_CMD("g") {
        char *regv = calloc(1, 17 * 8 + 1);
        TaskHandle_t th;
        th = AttachedTask;
        uint32_t *taskStack = (uint32_t *)(((uint32_t *)th)[1]);
        taskStack -= 17;
        //  1   2 ..17
        // CPSR R0-R15
        uint32_t val;

        for (int i = 1; i < 17; i++) {
            val = __builtin_bswap32(taskStack[i]);
            sprintf(&regv[(i - 1) * 8], "%08lX", val);
        }
        sprintf(&regv[16 * 8], "%08lX", __builtin_bswap32(taskStack[0]));
        sendPack(pcb, regv);
        free(regv);
        return;
    }

    sendPack(pcb, "");
}

static void recETX(struct tcp_pcb *pcb)
{
    
    sendPack(pcb, "S05");
}

static err_t TCPServerCallback(void *arg, struct tcp_pcb *pcb, struct pbuf *tcp_recv_pbuf, err_t err) {

    struct pbuf *tcp_send_pbuf;
    if (tcp_recv_pbuf != NULL) {
        tcp_recved(pcb, tcp_recv_pbuf->tot_len);

        for (int i = 0; i < tcp_recv_pbuf->tot_len; i++) {
            if(((char *)tcp_recv_pbuf->payload)[i] == 0x03)
            {
                printf("recv:ETX\r\n");
                recETX(pcb);
                continue;
            }
            // printf("%c", ((char *)tcp_recv_pbuf->payload)[i]);
            if (((char *)tcp_recv_pbuf->payload)[i] == '$') {
                startRec = 1;
                p = 0;
                continue;
            }
            if (((char *)tcp_recv_pbuf->payload)[i] == '#') {
                streamBuf[p] = 0;
                parse(pcb);
                startRec = 0;
                p = 0;
                continue;
            }
            if (startRec) {
                streamBuf[p++] = ((char *)tcp_recv_pbuf->payload)[i];
            }
        }
        // printf("\r\n");

        pbuf_free(tcp_recv_pbuf);

        // tcp_close(pcb);
    } else if (err == ERR_OK) {
        return tcp_close(pcb);
    }
    return ERR_OK;
}

static err_t TCPServerAccept(void *arg, struct tcp_pcb *pcb, err_t err) {
    tcp_recv(pcb, TCPServerCallback);
    return ERR_OK;
}

void GDB_Server_Initialization(void)

{
    struct tcp_pcb *tcp_server_pcb;
    tcp_server_pcb = tcp_new();
    err_t ret = tcp_bind(tcp_server_pcb, IP_ADDR_ANY, GDB_PORT);
    if (ret != ERR_OK) {
        printf("tcp bind err:%d\r\n", ret);
    }
    tcp_server_pcb = tcp_listen(tcp_server_pcb);
    tcp_accept(tcp_server_pcb, TCPServerAccept);
}

 
static int f(int k)
{
    
    if(k == 10)
    {
        //*((uint16_t *)0xF0000000) = 0x12;
    }
    if(k)
    {
        return k * f(k-1);
    }else{
        return 1;
    }
}


void test_task() {
    while (1) {
        f(30);
        vTaskDelay(pdMS_TO_TICKS(10));
        //printf("tick..\r\n");
    }
}

void gdb_server_init() 
{
    streamBuf = malloc(1024);
    sendBuf = malloc(2048);
 
    xTaskCreate(test_task, "testTask", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

    printf("Start gdb server\r\n");
    GDB_Server_Initialization();
}

void gdb_attach_to_task(TaskHandle_t task)
{
    AttachedTask = task;
}
