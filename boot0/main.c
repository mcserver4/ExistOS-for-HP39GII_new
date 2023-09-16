
// #include "FreeRTOS.h"
// #include "task.h"

#include "utils.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "clk.h"

#include "tusb.h"

#include "logo.h"


uint32_t ticks = 0;

uint32_t new_download = 0;
uint32_t get_free_mem();
void do_pending_irq() {
    uint32_t do_max = 16;
    uint32_t curIRQ = 0;
    cpu_disable_irq();
    do {
        if (irq_pendings) {
            if (irq_pending_ptr > 0) {
                irq_pending_ptr--;
            } else {
                irq_pending_ptr = sizeof(irq_ringbuf) - 1;
            }
            curIRQ = irq_ringbuf[irq_pending_ptr];
            if (curIRQ == 0xFF) {
                continue;
            } else {
                switch (curIRQ) {
                case HW_IRQ_TIMER0:

                    if (new_download > 0)
                        new_download--;
                    printf("TICK:%ld, free:%ld\r\n", ticks, get_free_mem());
                    ticks = 0;
                    break;

                case HW_IRQ_USB_CTRL:
                    dcd_int_handler(0);
                    break;

                default:
                    break;
                }
            }
            irq_ringbuf[irq_pending_ptr] = 0xFF;
            irq_pendings--;
        } else {
            break;
        }
    } while (do_max--);
    cpu_enable_irq();
}

#include "dhserver.h"
#include "dnserver.h"
#include "lwip/ethip6.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
/* lwip context */
static struct netif netif_data;

#define INIT_IP4(a, b, c, d)               \
    {                                      \
        PP_HTONL(LWIP_MAKEU32(a, b, c, d)) \
    }
/* network parameters of this MCU */
static const ip4_addr_t ipaddr = INIT_IP4(192, 168, 77, 1);
static const ip4_addr_t netmask = INIT_IP4(255, 255, 255, 0);
static const ip4_addr_t gateway = INIT_IP4(0, 0, 0, 0);

static err_t linkoutput_fn(struct netif *netif, struct pbuf *p) {
    (void)netif;

    for (;;) {
        /* if TinyUSB isn't ready, we must signal back to lwip that there is nothing we can do */
        if (!tud_ready())
            return ERR_USE;

        /* if the network driver can accept another packet, we make it happen */
        if (tud_network_can_xmit(p->tot_len)) {
            tud_network_xmit(p, 0 /* unused for this example */);
            return ERR_OK;
        }

        /* transfer execution to TinyUSB in the hopes that it will finish transmitting the prior packet */
        tud_task();
    }
}

static err_t ip4_output_fn(struct netif *netif, struct pbuf *p, const ip4_addr_t *addr) {
    return etharp_output(netif, p, addr);
}

static err_t netif_init_cb(struct netif *netif) {
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->mtu = CFG_TUD_NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->linkoutput = linkoutput_fn;
    netif->output = ip4_output_fn;
#if LWIP_IPV6
    netif->output_ip6 = ip6_output_fn;
#endif
    return ERR_OK;
}

static void init_lwip(void) {
    struct netif *netif = &netif_data;

    lwip_init();

    /* the lwip virtual MAC address must be different from the host's; to ensure this, we toggle the LSbit */
    netif->hwaddr_len = sizeof(tud_network_mac_address);
    memcpy(netif->hwaddr, tud_network_mac_address, sizeof(tud_network_mac_address));
    netif->hwaddr[5] ^= 0x01;

    netif = netif_add(netif, &ipaddr, &netmask, &gateway, NULL, netif_init_cb, ip_input);
#if LWIP_IPV6
    netif_create_ip6_linklocal_address(netif, 1);
#endif
    netif_set_default(netif);
}

/* shared between tud_network_recv_cb() and service_traffic() */
static struct pbuf *received_frame;

/* database IP addresses that can be offered to the host; this must be in RAM to store assigned MAC addresses */
static dhcp_entry_t entries[] =
    {
        /* mac ip address                          lease time */
        {{0}, INIT_IP4(192, 168, 77, 2), 24 * 60 * 60},
        {{0}, INIT_IP4(192, 168, 77, 3), 24 * 60 * 60},
        {{0}, INIT_IP4(192, 168, 77, 4), 24 * 60 * 60},
};

static const dhcp_config_t dhcp_config =
    {
        .router = INIT_IP4(0, 0, 0, 0),   /* router address (if any) */
        .port = 67,                       /* listen port */
        .dns = INIT_IP4(192, 168, 77, 1), /* dns server (if any) */
        "usb",                            /* dns suffix */
        TU_ARRAY_SIZE(entries),           /* num entry */
        entries                           /* entries */
};

/* handle any DNS requests from dns-server */
bool dns_query_proc(const char *name, ip4_addr_t *addr) {
    if (0 == strcmp(name, "tiny.usb")) {
        *addr = ipaddr;
        return true;
    }
    return false;
}

static void service_traffic(void) {
    /* handle any packet received by tud_network_recv_cb() */
    if (received_frame) {
        ethernet_input(received_frame, &netif_data);
        pbuf_free(received_frame);
        received_frame = NULL;
        tud_network_recv_renew();
    }

    sys_check_timeouts();
}

#include "lwip/apps/httpd.h"
/*
static u16_t test_ssi_handler(const char *ssi_tag_name, char *pcInsert, int iInsertLen) {

    if (strcmp(ssi_tag_name, "info") == 0)
        snprintf(pcInsert, iInsertLen, "Test:%ld", ticks);
    else
        return HTTPD_SSI_TAG_UNKNOWN;

    return strlen(pcInsert);
}
*/
#include "lfs.h"

int lfs_bd_read(const struct lfs_config *c, lfs_block_t block,
                lfs_off_t off, void *buffer, lfs_size_t size) {
    lfs_block_t blk = block + 48;
    uint32_t pg_off = off / (c->read_size);
    // printf("RD:%ld, %ld, size:%ld, b:%p\r\n", blk, off, size, buffer);
    assert(!(off % (c->read_size)));
    assert(!(size % (c->read_size)));
    assert(buffer);
    static lfs_block_t last_rd_blk = 0xFFFFFFFF;
    static bool last_rd_blk_is_bad = false;
    if (last_rd_blk != blk) {
        last_rd_blk = blk;
        last_rd_blk_is_bad = bsp_nand_check_is_bad_block(blk);
    }
    if (last_rd_blk_is_bad) {
        INFO("RD BAD BLOCK AT:%ld\r\n", blk);
        return LFS_ERR_CORRUPT;
    }
    bsp_nand_read_page_no_meta(blk * 64 + pg_off, buffer, NULL);
    return LFS_ERR_OK;
}

int lfs_bd_prog(const struct lfs_config *c, lfs_block_t block,
                lfs_off_t off, const void *buffer, lfs_size_t size) {
    lfs_block_t blk = block + 48;
    uint32_t pg_off = off / (c->prog_size);
    // printf("WR:%ld, %ld, size:%ld\r\n", blk, off, size);
    assert(!(off % (c->prog_size)));
    assert(!(size % (c->prog_size)));
    assert(buffer);
    static lfs_block_t last_rd_blk = 0xFFFFFFFF;
    static bool last_rd_blk_is_bad = false;
    if (last_rd_blk != blk) {
        last_rd_blk_is_bad = bsp_nand_check_is_bad_block(blk);
        last_rd_blk = blk;
    }
    if (last_rd_blk_is_bad) {
        INFO("WR BAD BLOCK AT:%ld\r\n", blk);
        return LFS_ERR_CORRUPT;
    }
    bsp_nand_write_page_nometa(blk * 64 + pg_off, (uint8_t *)buffer);
    return LFS_ERR_OK;
}

int lfs_bd_erase(const struct lfs_config *c, lfs_block_t block) {
    lfs_block_t blk = block + 48;
    static lfs_block_t last_rd_blk = 0xFFFFFFFF;
    static bool last_rd_blk_is_bad = false;
    if (last_rd_blk != blk) {
        last_rd_blk_is_bad = bsp_nand_check_is_bad_block(blk);
        last_rd_blk = blk;
    }
    if (last_rd_blk_is_bad) {
        INFO("ERASE BAD BLOCK AT:%ld\r\n", blk);
        return LFS_ERR_CORRUPT;
    }
    // printf("ERASE:%ld\r\n", blk);
    bsp_nand_erase_block(blk);
    return LFS_ERR_OK;
}

int lfs_bd_sync(const struct lfs_config *c) {

    return LFS_ERR_OK;
}
lfs_t lfs;
const struct lfs_config cfg = {
    // block device operations
    .read = lfs_bd_read,
    .prog = lfs_bd_prog,
    .erase = lfs_bd_erase,
    .sync = lfs_bd_sync,

    // block device configuration
    .read_size = (2048 + 0),
    .prog_size = (2048 + 0),
    .block_size = (2048 + 0) * 64,
    .block_count = 1024 - 48,

    .cache_size = 2048 + 0,
    .lookahead_size = 512,
    .block_cycles = 1000,
};

void do_conf() {
    void httpd_init(void);
    tud_init(BOARD_TUD_RHPORT);
    init_lwip();
    while (!netif_is_up(&netif_data))
        ;
    while (dhserv_init(&dhcp_config) != ERR_OK)
        ;
    while (dnserv_init(IP_ADDR_ANY, 53, dns_query_proc) != ERR_OK)
        ;
    httpd_init();
    // http_set_ssi_handler(test_ssi_handler, NULL, 0);

    bsp_display_put_string(0, 4 * 16, "USB RNDIS ON.");
    bsp_display_put_string(0, 5 * 16, "[Views] : Reboot");
    bsp_display_put_string(0, 6 * 16, "Connect to computer and access:");
    bsp_display_put_string(0, 7 * 16, "http://192.168.77.1/           ");

    while (1) {
        tud_task();
        service_traffic();
        ticks++;
        do_pending_irq();
        if(bsp_keyboard_is_key_down(KEY_VIEWS))
        {
            bsp_reset();
        }
    }
}

DMA_MEM_CHAINS uint32_t chains_info[5];

uint32_t boot_count = 0;
bool fs_init = false;
struct udp_pcb *udppcb = NULL; // = memp_malloc(MEMP_UDP_PCB);
int main() {
    bsp_board_init();
    cpu_enable_irq();

    print_uart0("UART TX Test\r\n");
    printf("Starting...\r\n");
    printf("CPUID:%08lx\r\n", get_cpu_id());
    bsp_timer0_set(1000000);

    
    //setHCLKDivider(2);
    //setCPUDivider(1);

    lfs_file_t file;
    int err = lfs_mount(&lfs, &cfg);
    // int err = 1;
    //  reformat if we can't mount the filesystem
    //  this should only happen on the first boot
    if (err) {
        fs_init = false;
        bsp_display_put_string(0, 0 * 16, "ERROR:");
        bsp_display_put_string(16, 2 * 16, "System is not installed.");
        bsp_display_put_string(16, 1 * 16, "NO File System.");
        do_conf();
        // lfs_format(&lfs, &cfg);
        // lfs_mount(&lfs, &cfg);
    } else {
        fs_init = true;
        bsp_display_put_string(0, 7 * 16, "Press [F2] to enter setup.");

        // read current count
        lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
        lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));
        printf("boot_count:%ld\r\n", boot_count);
        // update boot count
        boot_count += 1;
        lfs_file_rewind(&lfs, &file);
        lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

        // remember the storage is not updated until the file is closed successfully
        lfs_file_close(&lfs, &file);

        lfs_dir_t dir;
        struct lfs_info info;
        lfs_dir_open(&lfs, &dir, "/");
        printf("----lsdir----\r\n");
        while (lfs_dir_read(&lfs, &dir, &info) > 0) {
            printf("%s\r\n", info.name);
        }
        printf("----lsdir----\r\n");
        lfs_dir_close(&lfs, &dir);

        bsp_delayms(500);
        if (bsp_keyboard_is_key_down(KEY_F2)) {
            printf("Key F2 Pressed.\r\n");
            do_conf();
        }
    }

    if (lfs_file_open(&lfs, &file, "boot1.bin", LFS_O_RDONLY) == LFS_ERR_OK) {
        lfs_file_read(&lfs, &file, (void *)BOOT1_LOAD_ADDR, BOOT1_SIZE);
        lfs_file_close(&lfs, &file);
        lfs_unmount(&lfs);
        printf("JUMP BOOT1\r\n");
        bsp_diaplay_clean();
        bsp_display_put_string(0, 0 * 16, "Start BOOT1...");
        for (int i = 0; i < 10; i++) {
            printf("%08lX ", ((uint32_t *)BOOT1_LOAD_ADDR)[i]);
        }
        printf("\r\n");

        extern int chains_cmd;
        extern int chains_read;
        extern int chains_write;
        extern int chains_erase;
        extern int FlashSendCommandBuffer;

        chains_info[0] = (uint32_t)&chains_cmd;
        chains_info[1] = (uint32_t)&chains_read;
        chains_info[2] = (uint32_t)&chains_write;
        chains_info[3] = (uint32_t)&chains_erase;
        chains_info[4] = (uint32_t)&FlashSendCommandBuffer;
        bsp_timer0_set(0);
        bsp_usb_dcd_int_enable(0);

    
          setCPUDivider(1);   //  480 / 1 * (18/22) = 392 MHz
          setHCLKDivider(2);  //  392/2 = 196 MHz
          setCPUFracDivider(22);  

        
    
         // setCPUDivider(2);   //  480 / 2 * (18/22) = 196 MHz
         // setHCLKDivider(1);  //  196/1 = 196 MHz
         // setCPUFracDivider(22);  
//
       //  
    //
       //  setCPUDivider(4);   //  480 / 4 * (18/22) = 98 MHz
       //  setHCLKDivider(1);  //  98/1 = 98 MHz
       //  setCPUFracDivider(22);  

    //printf("StartCMTEst:\r\n");
        
    //int coremain();
    //coremain();
    
   // while(1);
        mmu_drain_buffer();
        mmu_invalidate_icache();
        mmu_clean_invalidated_dcache(BOOT1_LOAD_ADDR, BOOT1_SIZE);
        jump_boot1(chains_info);
    }

    bsp_display_put_string(0, 0 * 16, "ERROR:");
    bsp_display_put_string(16, 1 * 16, "boot1.bin not found!");

    do_conf();

    // udppcb = memp_malloc(MEMP_UDP_PCB);

    // udp_bind(udppcb, NULL, 5577);

    while (1)
        ;
}

#include "stmp3770_firmware.h"

#define ACTION_FLASH_BOOT0 1
#define ACTION_FLASH_BOOT1 2
#define ACTION_FLASH_SYS 3
#define ACTION_FLASH_ALL 4
#define ACTION_FLASH_HTTP_WRITE 5
#define ACTION_RESET 6

#define ACTION_FLASH_ERASE_ALL 10
#define ACTION_FLASH_SCAN_BADBLOCK 11
#define ACTION_FLASH_READBACK_ALL 12

#define ACTION_FLASH_HTTP_READ 13
#define ACTION_FLASH_HTTP_ERASE 14

uint32_t boot0_at_block, ldlb0_block, ldlb1_block, dbbt0_block, dbbt1_block;
uint32_t action = 0;
uint32_t post_len = 0;
uint32_t post_content_len = 0;
uint8_t post_stage = 0;

int nandhttp_block, nandhttp_off, nandhttp_size;
int nandhttp_cnt = 0, nandhttp_p = 0;

err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                       u16_t http_request_len, int content_len, char *response_uri,
                       u16_t response_uri_len, u8_t *post_auto_wnd) {
    // strcpy(response_uri,uri);

    post_content_len = content_len;
    //printf("post:%s\r\n", uri);
    if (memcmp(uri, "/nandwrite_block=", 17) == 0) {
        sscanf(uri, "/nandwrite_block=%d_off=%d_size=%d", &nandhttp_block, &nandhttp_off, &nandhttp_size);
        nandhttp_block += 48;
        //printf("HTTP NAND RD:%d,%d,%d\r\n", nandhttp_block, nandhttp_off, nandhttp_size);
        action = ACTION_FLASH_HTTP_WRITE;
        nandhttp_cnt = 0;
        post_stage = 3;
    }

    if (strcmp(uri, "/flash_boot1") == 0) {
        if (!fs_init) {
            printf("INIT FS1\r\n");
            lfs_format(&lfs, &cfg);
            lfs_mount(&lfs, &cfg);
            fs_init = true;
        }
        action = ACTION_FLASH_BOOT1;
        post_stage = 0;
    }

    if (strcmp(uri, "/flash_sys") == 0) {
        if (!fs_init) {
            printf("INIT FS2\r\n");
            lfs_format(&lfs, &cfg);
            lfs_mount(&lfs, &cfg);
            fs_init = true;
        }
        action = ACTION_FLASH_SYS;
        post_stage = 0;
    }

    if (strcmp(uri, "/flash_boot0") == 0) {
        for (boot0_at_block = 32; boot0_at_block < 48; boot0_at_block++) {
            if (!bsp_nand_check_is_bad_block(boot0_at_block)) {
                break;
            }
        }
        bool ncb0 = false;
        bool ncb1 = false;
        bool dbbt0 = false;
        bool dbbt1 = false;
        bool ldlb0 = false;
        bool ldlb1 = false;

        for (int i = 0; i < 32; i += 4) {
            if ((!bsp_nand_check_is_bad_block(i)) && (!ncb0)) {
                mkNCB(i);
                ncb0 = true;
                continue;
            }
            if ((!bsp_nand_check_is_bad_block(i)) && (!ncb1)) {
                mkNCB(i);
                ncb1 = true;
                continue;
            }
            if ((!bsp_nand_check_is_bad_block(i)) && (!ldlb0)) {
                ldlb0_block = i;
                ldlb0 = true;
                continue;
            }
            if ((!bsp_nand_check_is_bad_block(i)) && (!ldlb1)) {
                ldlb1_block = i;
                ldlb1 = true;
                continue;
            }
            if ((!bsp_nand_check_is_bad_block(i)) && (!dbbt0)) {
                mkDBBT(i);
                dbbt0_block = i;
                dbbt0 = true;
                continue;
            }
            if ((!bsp_nand_check_is_bad_block(i)) && (!dbbt1)) {
                mkDBBT(i);
                dbbt1_block = i;
                dbbt1 = true;
                continue;
            }
        }
        action = ACTION_FLASH_BOOT0;
        post_stage = 0;
    }

    if (strcmp(uri, "/flash_all") == 0) {
        action = ACTION_FLASH_ALL;
        post_stage = 0;
    }

    if (strcmp(uri, "/flash_scan") == 0) {
        action = ACTION_FLASH_SCAN_BADBLOCK;
        post_stage = 0;
    }

    if (strcmp(uri, "/flash_erase") == 0) {
        action = ACTION_FLASH_ERASE_ALL;
        for (int i = 0; i < 1024; i++) {
            if ((i % 128) == 0)
                printf("ERASE:%d\r\n", i);
            bsp_nand_erase_block(i);
        }
        fs_init = false;
    }

    post_len = 0;
    return ERR_OK;
}

uint8_t wr_pg_buf[2048 + 19];

void post_stream_in(uint8_t dat, bool fin, bool post_start) {

    switch (action) {
    case ACTION_FLASH_HTTP_WRITE: {
        static uint32_t ptr = 0;
        if(post_start)
        {
            ptr = 0;
        }
        wr_pg_buf[ptr++] = dat;
        if((ptr >= 2048) || fin)
        {
            ptr = 0;
            bsp_nand_write_page_nometa(nandhttp_block * 64 + (nandhttp_off / 2048) + nandhttp_cnt,wr_pg_buf);
            nandhttp_cnt++;
        }
        
        break;
    }
    case ACTION_FLASH_SYS:
    case ACTION_FLASH_BOOT1: {
        static lfs_file_t f;
        static uint32_t ptr = 0;
        if (post_start) {
            // printf("CPU MODE:%02lX\r\n",get_mode());
            ptr = 0;
            int fopenret = lfs_file_open(&lfs, &f, action == ACTION_FLASH_BOOT1 ? "boot1.bin" : "sys.bin", LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC);
            printf("fopens:%d\r\n", fopenret);
        }
        wr_pg_buf[ptr++] = dat;
        if (ptr >= 2048 + 0) {
            lfs_file_write(&lfs, &f, wr_pg_buf, 2048 + 0);
            ptr = 0;
        }
        // lfs_file_write(&lfs, &f, &dat, 1);
        if (fin) {
            lfs_file_write(&lfs, &f, wr_pg_buf, ptr);

            int fsclose = lfs_file_close(&lfs, &f);
            printf("fsclose:%d\r\n", fsclose);
        }
        break;
    }
    case ACTION_FLASH_BOOT0: {
        static int p = 0;
        static int boot0_length = 0;
        static int pgs = 0;
        if (post_start) {
            pgs = 0;
            p = 0;
            memset(wr_pg_buf, 0xFF, sizeof(wr_pg_buf));
            bsp_nand_erase_block(boot0_at_block);
        }
        wr_pg_buf[p] = dat;
        p++;
        boot0_length++;
        if ((p >= 2048) || fin) {

            wr_pg_buf[2048 + 1] = pgs / 64;
            wr_pg_buf[2048 + 2] = 0x53; // S
            wr_pg_buf[2048 + 3] = 0x54; // T
            wr_pg_buf[2048 + 4] = 0x4D; // M
            wr_pg_buf[2048 + 5] = 0x50; // P

            bsp_nand_write_page(boot0_at_block * 64 + pgs, wr_pg_buf);
            pgs++;
            if (pgs % 64 == 0) {
                bsp_nand_erase_block(boot0_at_block + pgs / 64);
            }
            p = 0;
            memset(wr_pg_buf, 0xFF, sizeof(wr_pg_buf));
            if (fin) {
                printf("MK LDLB \r\n");
                mkLDLB(ldlb0_block, boot0_at_block * 64, (boot0_length + 2047) / 2048, dbbt0_block * 64, dbbt1_block * 64);
                mkLDLB(ldlb1_block, boot0_at_block * 64, (boot0_length + 2047) / 2048, dbbt0_block * 64, dbbt1_block * 64);
            }
        }
        break;
    }
    case ACTION_FLASH_ALL: {
        static int skip = 0;
        static int pgs = -1;
        static int pg_ptr = 0;
        static int empty = 0;
        if (post_start) {
            pg_ptr = 0;
            skip = 0;
            pgs = -1;
            empty = 0;
        }
        if (skip) {
            wr_pg_buf[pg_ptr++] = dat;
            skip--;
            return;
        } else {
            if ((pgs >= 0) && (!empty)) {
                bsp_nand_write_page(pgs, wr_pg_buf);
            }
        }
        if (dat == 0xCE) {
            pg_ptr = 0;
            skip = 2048 + 19;
            pgs++;
            empty = 0;
        } else if (dat == 0xF7) {
            skip = 0;
            pgs++;
            empty = 1;
        }
        if (fin) {
            printf("pgs:%d\r\n", pgs);
        }
        break;
    }

    default:
        break;
    }
}

uint8_t post_rec_buf2[256];
uint8_t post_rec_buf[1536];
char boundary[72];
err_t httpd_post_receive_data(void *connection, struct pbuf *p) {
    // struct http_state *hs = (struct http_state *)connection;

    if (action >= ACTION_FLASH_ERASE_ALL) {
        pbuf_free(p);
        return ERR_OK;
    }

    struct pbuf *q = p;
    int cur_ptr = 0;

    while (q != NULL) {
        assert(q->len < sizeof(post_rec_buf));
        memcpy(&post_rec_buf[cur_ptr], q->payload, q->len);
        post_len += q->len;
        cur_ptr += q->len;
        q = q->next;
    }

    int sep = 0;
    int last_i = 0;
    int post_fin = 0;
    uint32_t dat_start = 0;
    switch (post_stage) {
    case 0:
        //
        for (int i = 0; i < cur_ptr; i++) {
            // printf("%02X ", post_rec_buf[i]);
            memset(post_rec_buf2, 0, sizeof(post_rec_buf2));
            if ((post_rec_buf[i] == 0x0D) && (post_rec_buf[i + 1] == 0x0A)) {
                switch (sep) {
                case 0:
                    last_i = i + 2;
                    memset(boundary, 0, sizeof(boundary));
                    memcpy(boundary, post_rec_buf, i);
                    printf("0:boundary:[%s]\r\n", boundary);
                    sep = 1;
                    break;
                case 1:
                    memcpy(post_rec_buf2, &post_rec_buf[last_i], i - last_i);
                    printf("1:Content-Disposition:[%s]\r\n", post_rec_buf2);
                    sep = 2; 
                    last_i = i + 2;
                    break;
                case 2:
                    memcpy(post_rec_buf2, &post_rec_buf[last_i], i - last_i);
                    printf("2:Content-Type:[%s]\r\n", post_rec_buf2);
                    sep = 3;
                    last_i = i + 2;
                    break;
                case 3:
                    post_stage = 1;
                    dat_start = i + 2;
                    break;
                default:
                    break;
                }
                if (post_stage == 1) {
                    break;
                }
            }
        }
        printf("\r\n");
        // break;

    case 1: {
        if (post_content_len - post_len == 0) {
            for (int i = cur_ptr - 3; i > 0; i--) {
                if ((post_rec_buf[i] == 0x0D) && (post_rec_buf[i + 1] == 0x0A)) {
                    // printf("RECB2:[%s]\r\n", &post_rec_buf[i + 2]);
                    if (memcmp(boundary, &post_rec_buf[i + 2], strlen(boundary)) == 0) {
                        printf("post END\r\n");
                        post_fin = 1;
                        cur_ptr = i;
                    }
                }
            }
        }

        if ((!post_fin) && (!(post_content_len - post_len))) {
            post_fin = 1;
        }

        for (int i = dat_start; i < cur_ptr; i++) {
            // printf("%02X ", post_rec_buf[i]);
            post_stream_in(post_rec_buf[i], (post_fin) && (i == (cur_ptr - 1)), i == dat_start);
        }
        // printf("\r\n");
        post_stage = 2;
        break;
    }

    case 2: {
        // printf("remind:%d\r\n",post_content_len - post_len);
        if (post_content_len - post_len == 0) {
            for (int i = cur_ptr - 3; i > 0; i--) {
                if ((post_rec_buf[i] == 0x0D) && (post_rec_buf[i + 1] == 0x0A)) {
                    // printf("RECB2:[%s]\r\n", &post_rec_buf[i + 2]);
                    if (memcmp(boundary, &post_rec_buf[i + 2], strlen(boundary)) == 0) {
                        printf("post END\r\n");
                        post_fin = 1;
                        cur_ptr = i;
                    }
                }
            }
        }
        if ((!post_fin) && (!(post_content_len - post_len))) {
            post_fin = 1;
        }

        for (int i = 0; i < cur_ptr; i++) {
            // printf("%02X ", post_rec_buf[i]);
            post_stream_in(post_rec_buf[i], (post_fin) && (i == (cur_ptr - 1)), false);
        }
        // printf("\r\n");
        break;
    }

    case 3: {
        dat_start = 0;
        if ((!(post_content_len - post_len))) {
            post_fin = 1;
        }
        for (int i = dat_start; i < cur_ptr; i++) {
            post_stream_in(post_rec_buf[i], (post_fin) && (i == (cur_ptr - 1)), i == dat_start);
        }
        post_stage = 4;
        break;
    }
    case 4: {
        if ((!(post_content_len - post_len))) {
            post_fin = 1;
        }
        for (int i = 0; i < cur_ptr; i++) {
            post_stream_in(post_rec_buf[i], (post_fin) && (i == (cur_ptr - 1)), false);
        }
    }

    default:
        break;
    }

    // printf("\r\n");
    // printf("recLen:%d, Remind:%ld\r\n",cur_ptr, post_content_len - post_len);

    pbuf_free(p);

    return ERR_OK;
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len) {
    printf("post_len:%ld, Remind:%ld\r\n", post_len, post_content_len - post_len);

    if (action == ACTION_FLASH_SCAN_BADBLOCK) {
        strcpy(response_uri, "/scan.html");
    } else {
        strcpy(response_uri, "/done.html");
    }
    action = 0;
}

#include <lwip/apps/fs.h>
#include <lwip/mem.h>

int cur_scan_blk = 0;
int badBlks = 0;

int fs_open_custom(struct fs_file *file, const char *name) {
    printf("custom open:%s\r\n", name);



    if (memcmp(name, "/nandread_block=", 16) == 0) {
        sscanf(name, "/nandread_block=%d_off=%d_size=%d", &nandhttp_block, &nandhttp_off, &nandhttp_size);
        nandhttp_block += 48;
        //printf("HTTP NAND RD:%d,%d,%d\r\n", nandhttp_block, nandhttp_off, nandhttp_size);
        file->len = nandhttp_size;
        file->pextension = (void *)0;
        file->index = 0;
        action = ACTION_FLASH_HTTP_READ;
        nandhttp_cnt = 0;
        return 1;
    }

    if (memcmp(name, "/nanderase_block=", 17) == 0) {
        sscanf(name, "/nanderase_block=%d", &nandhttp_block);
        nandhttp_block += 48;
        //printf("HTTP NAND ERASE:%d\r\n", nandhttp_block);
        file->len = 1;
        file->pextension = (void *)0;
        file->index = 0;
        action = ACTION_FLASH_HTTP_ERASE;
        nandhttp_cnt = 0;
        return 1;
    }

    if (strcmp(name, "/flash_backup.ebk") == 0) {
        if (new_download != 0) {
            return 0;
        }
        new_download = 5;
        action = ACTION_FLASH_READBACK_ALL;

        file->len = (1 + 2048 + 19);
        file->pextension = (void *)0;
        return 1;
    }

    if (strcmp(name, "/scan.html") == 0) {
        cur_scan_blk = 0;
        badBlks = 0;
        action = ACTION_FLASH_SCAN_BADBLOCK;
        file->len = 2048;
        return 1;
    }

    if (strcmp(name, "/reset") == 0) {
        action = ACTION_RESET;
        file->len = 4;
        return 1;
    }

    return 0;
}

void fs_close_custom(struct fs_file *file) {
    if(action == ACTION_RESET)
    {
        if(fs_init)
        {
            lfs_unmount(&lfs);
        }
        bsp_delayms(500);
        bsp_reset();
    }
    action = 0;
}

uint8_t rdpg[2048 + 19];
int fs_read_custom(struct fs_file *file, char *buffer, int count) {
    if(action == ACTION_RESET)
    {
        file->index = file->len;
        sprintf(buffer,"Done");
        return 4;
    }
    if (action == ACTION_FLASH_HTTP_ERASE) {
        if (nandhttp_cnt >= 1) {
            return -1;
        }
        bsp_nand_erase_block(nandhttp_block);
        buffer[0] = 'T';
        nandhttp_cnt++;
        return 1;
    }
    if (action == ACTION_FLASH_HTTP_READ) {
        // printf("count:%d\r\n",count);

        if (nandhttp_cnt * 2048 > nandhttp_size) {
            return -1;
        }

        bsp_nand_read_page_no_meta(nandhttp_block * 64 + nandhttp_cnt + (nandhttp_off / 2048), rdpg, NULL);
        memcpy(buffer, rdpg, count > 2048 ? 2048 : count);
        nandhttp_cnt++;
        if (nandhttp_cnt * 2048 >= nandhttp_size) {
            nandhttp_cnt++;
        }
        return 2048;
    } else if (action == ACTION_FLASH_SCAN_BADBLOCK) {
        char text[128];
        int i = 0;
        if (cur_scan_blk > 1024 + 1) {
            return FS_READ_EOF;
        }

        buffer[0] = 0;

        do {
            if (bsp_nand_check_is_bad_block(cur_scan_blk)) {
                badBlks++;
                sprintf(text, "<h3 style=\"color:red\">Found bad block at:%d</h3>", cur_scan_blk);
                memcpy(&buffer[i], text, strlen(text));
                i = i + strlen(text);
            }
            cur_scan_blk++;
        } while ((i < count) && (cur_scan_blk < 1024));

        if (cur_scan_blk >= 1024) {
            cur_scan_blk++;
            sprintf(text, "<br /><h1>Scan Finish. Found %d bad block(s).</h1><br /><a href=\"/\"> back </a>", badBlks);
            if ((i + strlen(text)) < count) {
                memcpy(&buffer[i], text, strlen(text));
                cur_scan_blk++;
                return i + strlen(text);
            }
        }
    } else if (action == ACTION_FLASH_READBACK_ALL) {
        uint32_t ECC = 0;
        uint32_t pg = (uint32_t)file->pextension;

        assert(count == 1 + sizeof(rdpg));

        if (pg >= 65536) {
            return FS_READ_EOF;
        }
        int i = 0;
        do {
            new_download = 5;
            action = ACTION_FLASH_READBACK_ALL;
            bsp_nand_read_page(pg++, rdpg, &ECC);
            if (ECC == 0x0F0F0F0F) {
                buffer[i++] = 0xF7;
                if (pg >= 65536) {
                    file->pextension = (void *)pg;
                    return i;
                }
            } else {
                if (i == 0) {
                    buffer[0] = 0xCE;
                    memcpy(&buffer[1], rdpg, sizeof(rdpg));
                    file->pextension = (void *)pg;
                    return count;
                } else {
                    file->pextension = (void *)(pg - 1);
                    return i;
                }
            }
        } while (i < count);
        file->pextension = (void *)pg;
        return count;
    }

    return -1;
}



void dcd_dcache_clean(void const *addr, uint32_t data_size) {
    //(void) addr; (void) data_size;
    // mmu_clean_dcache((uint32_t)addr, (uint32_t)data_size);
}

void dcd_dcache_invalidate(void const *addr, uint32_t data_size) {
    //(void) addr; (void) data_size;
    // mmu_invalidate_dcache((uint32_t)addr, data_size);
}

void dcd_dcache_clean_invalidate(void const *addr, uint32_t data_size) {
    (void)addr;
    (void)data_size;
    // mmu_clean_invalidated_dcache((uint32_t)addr, data_size);
}

#include "dhserver.h"
#include "dnserver.h"
#include "lwip/ethip6.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"

uint8_t tud_network_mac_address[6] = {0x02, 0x02, 0x84, 0x6A, 0x96, 0x00};

void tud_network_init_cb(void) {
    /* if the network is re-initializing and we have a leftover packet, we must do a cleanup */
    if (received_frame) {
        pbuf_free(received_frame);
        received_frame = NULL;
    }
}

bool tud_network_recv_cb(const uint8_t *src, uint16_t size) {
    /* this shouldn't happen, but if we get another packet before
    parsing the previous, we must signal our inability to accept it */
    if (received_frame)
        return false;

    if (size) {
        struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);

        if (p) {
            /* pbuf_alloc() has already initialized struct; all we need to do is copy the data */
            memcpy(p->payload, src, size);

            /* store away the pointer for service_traffic() to later handle */
            received_frame = p;
        }
    }

    return true;
}

uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg) {
    struct pbuf *p = (struct pbuf *)ref;

    (void)arg; /* unused for this example */

    return pbuf_copy_partial(p, dst, p->tot_len, 0);
}
