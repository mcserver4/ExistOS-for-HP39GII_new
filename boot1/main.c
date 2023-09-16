#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "utils.h"

#include "mm.h"

#include "bsp.h" 

#include "lfs.h"

lfs_t lfs;

int lfs_bd_read(const struct lfs_config *c, lfs_block_t block,
                lfs_off_t off, void *buffer, lfs_size_t size) {
    lfs_block_t blk = block + 48;
    uint32_t pg_off = off / (c->read_size);
    // printf("RD:%ld, %ld, size:%ld, b:%p\r\n", blk, off, size, buffer);
    assert(!(off % (c->read_size)));
    assert(!(size % (c->read_size)));
    assert(buffer);
    //static lfs_block_t last_rd_blk = 0xFFFFFFFF;
    //static bool last_rd_blk_is_bad = false;
    //if (last_rd_blk != blk) {
    //    last_rd_blk_is_bad = bsp_nand_check_is_bad_block(blk);
    //    last_rd_blk = blk;
    //}
    //if (last_rd_blk_is_bad)
    //    return LFS_ERR_CORRUPT;

    uint8_t *target = buffer;
    uint32_t pgs = size / (c->read_size);
    for (int i = 0; i < pgs; i++) {
        bsp_nand_read_page_nometa(blk * 64 + pg_off + i, &target[i * c->read_size], NULL);
    }
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
    //static lfs_block_t last_rd_blk = 0xFFFFFFFF;
    //static bool last_rd_blk_is_bad = false;
    //if (last_rd_blk != blk) {
    //    last_rd_blk_is_bad = bsp_nand_check_is_bad_block(blk);
    //    last_rd_blk = blk;
    //}
    //if (last_rd_blk_is_bad)
    //    return LFS_ERR_CORRUPT;

    const uint8_t *target = buffer;
    uint32_t pgs = size / (c->read_size);
    for (int i = 0; i < pgs; i++) {
        bsp_nand_write_page_nometa(blk * 64 + pg_off + i, (void *)&target[i * c->read_size]);
    }
    return LFS_ERR_OK;
}

int lfs_bd_erase(const struct lfs_config *c, lfs_block_t block) {
    lfs_block_t blk = block + 48;
    // printf("ERASE:%ld\r\n", blk);
    bsp_nand_erase_block(blk);
    return LFS_ERR_OK;
}

int lfs_bd_sync(const struct lfs_config *c) {

    return LFS_ERR_OK;
}

const struct lfs_config cfg = {
    // block device operations
    .read = lfs_bd_read,
    .prog = lfs_bd_prog,
    .erase = lfs_bd_erase,
    .sync = lfs_bd_sync,

    // block device configuration
    .read_size = (2048),
    .prog_size = (2048),
    .block_size = (2048) * 64,
    .block_count = 1024 - 48,

    .cache_size = FS_CACHE_SZ,
    .lookahead_size = 8,
    .block_cycles = 1000,
};

lfs_t lfs;

extern volatile uint32_t iocnt_read;
extern volatile uint32_t iocnt_write;
extern volatile uint32_t iocnt_erase;



/*

uint8_t testPage[2048] __aligned(4);
void raw_io_test() {
    srand(2);

    INFO("SPEED TEST START:\r\n");

    for (int i = 0; i < sizeof(testPage); i++) {
        testPage[i] = rand();
    }

    for (int i = 48; i < 1024; i++) {
        bsp_nand_erase_block(i);
    }

    INFO("SPEED TEST START:\r\n");
    uint32_t stime = bsp_time_get_ms();
    for (int i = 48 * 64, j=0; i < 65500; i++, j++) {
        bsp_nand_write_page(i, testPage);
        if (bsp_time_get_ms() - stime > 1000) {
            printf("WR SPEED:%dKB/s, IOR:%ld, IOW:%ld, IOE:%ld\r\n", sizeof(testPage) * j / 1024, iocnt_read, iocnt_write, iocnt_erase);
            iocnt_write = 0;
            iocnt_erase = 0;
            iocnt_read = 0;
            stime = bsp_time_get_ms();
            j = 0;
        }
    }

    INFO("SPEED TEST START:\r\n");
    // lfs_file_t f;
    stime = bsp_time_get_ms();
    for  (int i = 48 * 64, j=0; i < 65500; i++, j++) {
        bsp_nand_read_page(i, testPage, NULL);
        if (bsp_time_get_ms() - stime > 1000) {
            printf("SEQ RD SPEED:%dKB/s, IO:%ld\r\n", sizeof(testPage) * j / 1024, iocnt_read);
            iocnt_read = 0;
            stime = bsp_time_get_ms();
            j = 0;
        }
    }
    INFO("SPEED TEST START:\r\n");
    // lfs_file_t f;
    stime = bsp_time_get_ms();
    for (int i = 48 * 64, j=0; i < 65500; i++, j++) {
        //lfs_file_seek(&lfs, &f, rand() & (1 * 1048576 - 1), LFS_SEEK_SET);
        bsp_nand_read_page(rand() % (65500), testPage, NULL);
        if (bsp_time_get_ms() - stime > 1000) {
            printf("RAND RD SPEED:%dKB/s, IO:%ld\r\n", sizeof(testPage) * j / 1024, iocnt_read);
            iocnt_read = 0;
            stime = bsp_time_get_ms();
            j = 0;
        }
    }
    INFO("TEST FIN\r\n");
}
*/
/*
void lfs_speed_test() {
    srand(2);
    for (int i = 0; i < sizeof(testPage); i++) {
        testPage[i] = rand();
    }

    INFO("SPEED TEST START:\r\n");
    lfs_file_t f;
    lfs_file_open(&lfs, &f, "/test", LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC);
    uint32_t stime = bsp_time_get_ms();
    for (uint32_t i = 0, j = 0; i < 4096; i++, j++) {
        lfs_file_write(&lfs, &f, testPage, sizeof(testPage));
        if (bsp_time_get_ms() - stime > 1000) {
            printf("WR SPEED:%ldKB/s, IOR:%ld, IOW:%ld, IOE:%ld\r\n", sizeof(testPage) * j / 1024, iocnt_read, iocnt_write, iocnt_erase);
            iocnt_write = 0;
            iocnt_erase = 0;
            iocnt_read = 0;
            stime = bsp_time_get_ms();
            j = 0;
        }
    }
    lfs_file_close(&lfs, &f);

    INFO("SPEED TEST START:\r\n");
    // lfs_file_t f;
    lfs_file_open(&lfs, &f, "/test", LFS_O_RDONLY);
    lfs_file_rewind(&lfs, &f);
    stime = bsp_time_get_ms();
    for (uint32_t i = 0, j = 0; i < 4096; i++, j++) {
        lfs_file_seek(&lfs, &f, i*sizeof(testPage), LFS_SEEK_SET);
        lfs_file_read(&lfs, &f, testPage, sizeof(testPage));
        if (bsp_time_get_ms() - stime > 1000) {
            printf("SEQ RD SPEED:%ldKB/s, IO:%ld\r\n", sizeof(testPage) * j / 1024, iocnt_read);
            iocnt_read = 0;
            stime = bsp_time_get_ms();
            j = 0;
        }
    }
    lfs_file_close(&lfs, &f);
    INFO("TEST FIN\r\n");

    INFO("SPEED TEST START:\r\n");
    // lfs_file_t f;
    lfs_file_open(&lfs, &f, "/test", LFS_O_RDONLY);
    lfs_file_rewind(&lfs, &f);
    stime = bsp_time_get_ms();
    for (uint32_t i = 0, j = 0; i < 4096; i++, j++) {
        lfs_file_seek(&lfs, &f, rand() & (1 * 1048576 - 1), LFS_SEEK_SET);
        lfs_file_read(&lfs, &f, testPage, sizeof(testPage));
        if (bsp_time_get_ms() - stime > 1000) {
            printf("RAND RD SPEED:%ldKB/s\r\n", sizeof(testPage) * j / 1024);
            stime = bsp_time_get_ms();
            j = 0;
        }
    }
    lfs_file_close(&lfs, &f);
    INFO("TEST FIN\r\n");
}
*/
 

/*
uint32_t _randSeed = 1;

uint32_t _rand(void)
{
    uint32_t value;
    //Use a linear congruential generator (LCG) to update the state of the PRNG
    _randSeed *= 1103515245;
    _randSeed += 12345;
    value = (_randSeed >> 16) & 0x07FF;
    _randSeed *= 1103515245;
    _randSeed += 12345;
    value <<= 10;
    value |= (_randSeed >> 16) & 0x03FF;
    _randSeed *= 1103515245;
    _randSeed += 12345;
    value <<= 10;
    value |= (_randSeed >> 16) & 0x03FF;
    //Return the random value
    return value;
}



void memTestWrite(uint32_t base, uint32_t length, uint32_t seed)
{
    uint32_t val;
    uint8_t *p = (uint8_t *)base;
    
    _randSeed = seed;
    printf("memtest start\r\n");

    for(int i = 0; i < length;i++)
    {
        val = _rand() & 0xFF;        
        if(i < 20)
        {
            printf("TST WR:%02lX\r\n", val);
        }
        p[i] = val;
    }
    
    printf("memtest write fin\r\n");
}

void memTestRead(uint32_t base, uint32_t length, bool trim, uint32_t seed)
{
    uint32_t val;
    uint8_t *p = (uint8_t *)base; 
    
    _randSeed = seed;
    for(int i = 0; i < length;i++)
    {
        val = _rand() & 0xFF;
        if(i < 20)
        {
            printf("TST RD:%02lX\r\n", val);
        }
        if(p[i] != val)
        {
            printf("MEM ERR:%p, %lX == %X\r\n",&p[i],val,p[i]);
        }
    }

    if(trim)
        for(int i = 0; i < length;i+=1024)
            mm_trim_page((uint32_t)&p[i]); 
 

    printf("memtest fin\r\n");
}
void mmap_test()
{
    //lfs_file_t testf;
    //lfs_file_open(&lfs, &testf, "mmaptest", LFS_O_CREAT | LFS_O_TRUNC | LFS_O_RDWR);
    //lfs_file_seek(&lfs, &testf, 2*1048576 - 1, LFS_SEEK_SET);
    //lfs_file_write(&lfs, &testf, "c", 1);
    //lfs_file_close(&lfs, &testf);


    int ret = mmap(0x30000000, "mmaptest", 0, 0, true, true);
    printf("mmap tst:%d\r\n", ret);


    //memTestWrite(0x30000000, 2*1048576, 112317 );
    //memTestWrite(MAIN_MEMORY_BASE, 400000, 7749 );

    
    //memTestRead(0x30000000, 2*1048576, 0,112317 );
    //memTestRead(MAIN_MEMORY_BASE, 400000,1, 7749 );

    //memTestWrite(0x30000400, 1024*500, 112317 );
    memTestWrite(0x30000000, 1024*500, 1112 );
    memTestWrite(MAIN_MEMORY_BASE, 400000, 7749 );
    memTestRead(MAIN_MEMORY_BASE, 400000, 1, 7749 );
    //memTestRead(0x30000400,  1024*500, 0,112317 );
    memTestRead( 0x30000000,  1024*500, 1,1112 );

}

void __attribute__((target("arm")))  test_swi()
{

    for(int i = 0; i < 8000; i++)
    {
        __asm volatile("push {r0}");
        __asm volatile("add r0,r0,#1");
        __asm volatile("SWI 0x5A");
        __asm volatile("pop {r0}");
    }
}



*/
uint32_t abt_stack_base = ABT_STACK_VADDR;

//struct mallinfo minfo;
#include "umm_malloc.h"
#include "umm_malloc_cfg.h"


#define SUPPORT_UMM_MALLOC_BLOCKS (16*1024 / 8)
char umm_heap[SUPPORT_UMM_MALLOC_BLOCKS][UMM_BLOCK_BODY_SIZE] __aligned(4);;
void *UMM_MALLOC_CFG_HEAP_ADDR = &umm_heap;
#define SUPPORT_UMM_MALLOC_HEAP_SIZE (SUPPORT_UMM_MALLOC_BLOCKS * UMM_BLOCK_BODY_SIZE)
uint32_t UMM_MALLOC_CFG_HEAP_SIZE = SUPPORT_UMM_MALLOC_HEAP_SIZE;

int main(int par, char **argc) {

    umm_init();
    
    print_uart0("START BOOT1.\r\n");
    printf("AT:%08X\r\n", par);
    bsp_nand_init(((uint32_t *)par));
    
    int ret = 0;


    ret = lfs_mount(&lfs, &cfg);
    if (ret == LFS_ERR_OK) {
        lfs_dir_t dir;
        struct lfs_info info;
        lfs_dir_open(&lfs, &dir, "/");
        printf("----lsdir----\r\n");
        while (lfs_dir_read(&lfs, &dir, &info) > 0) {
            printf("%s\r\n", info.name);
        }
        printf("----lsdir----\r\n");
        lfs_dir_close(&lfs, &dir);
    }
    printf("LFS MOUNT:%d\r\n", ret);

    //printf("mkdirt:%d\r\n", lfs_mkdir(&lfs,"testdir"));

    if(ret == LFS_ERR_OK)
    {
        printf("Free blocks:%ld\r\n", cfg.block_count - lfs_fs_size(&lfs));
    }

    //lfs_file_t memf;
    //lfs_file_open(&lfs, &memf, "/memf", LFS_O_CREAT | LFS_O_TRUNC | LFS_O_RDWR);
    //lfs_file_seek(&lfs, &memf, 2*1048576 - 1, LFS_SEEK_SET);
    //lfs_file_write(&lfs, &memf, "c", 1);
    //lfs_file_close(&lfs, &memf);


    //lfs_file_t kf, skf;
    //uint32_t buf;
    //printf("Start copy\r\n");
    //lfs_file_open(&lfs, &skf, "/xipImage", LFS_O_RDONLY);
    //lfs_file_seek(&lfs, &skf, 0, LFS_SEEK_SET);
    //lfs_file_open(&lfs, &kf, "/xipImage_m", LFS_O_CREAT | LFS_O_TRUNC | LFS_O_RDWR);
//
    //while(lfs_file_read(&lfs, &skf, &buf, 4) > 0)
    //{
    //    lfs_file_write(&lfs, &kf, &buf, 4);
    //}
//
    //lfs_file_close(&lfs, &skf);
    //lfs_file_close(&lfs, &kf);
    //printf("copy fin\r\n");

 
    mm_init();

    // lfs_speed_test();
    // volatile uint32_t *p = (uint32_t *)MAIN_MEMORY_BASE;
    // int i = 0;
    //mmap(0x00800000, "/memf", 0, 2*1048576, true, true);

    //mmap(0x00400000, "/xipImage_m", 0*1048576, 1048576, true, true);
    //mmap(0x00500000, "/xipImage_m", 1*1048576, 1048576, true, true);
   // mmap(0x00600000, "/xipImage_m", 2*1048576, 0, true, true);
    

    //lfs_speed_test();while(1);
 

    ret = mmap(0x00100000, "sys.bin", 0, 0, false, false);

    
    printf("mmap sys:%d\r\n", ret);

    
    //printf("B Free:%ld\r\n", minfo.fordblks + get_free_mem());

   
    //__asm volatile("SWI 0x5A");

    printf("DUMP SYS:\r\n");
    for(int i =0; i < 20; i ++)
    {
        printf("%08lX ", ((uint32_t *)0x00100000)[i]);
    }
    printf("\r\n");
    //__asm volatile("SWI 0x5A");

    //mmap_test();


    

    switch_mode(SYS_MODE);
    //test_swi();

    __asm volatile("ldr r0,=0x00100000");
    __asm volatile("bx r0");

   // while(1);

    while (1) {
       //minfo = mallinfo();

       //printf("Alloc:%d\r\n", minfo.uordblks);
       //printf("Free:%ld\r\n", minfo.fordblks + get_free_mem());
       // bsp_delayms(1000);

        // for(;;)
        {
            //*p = i++;
            // if(((uint32_t)p) % 8192 == 0)
            // printf("%p:%08lx\r\n", p, *p);
            // p++;
        }
        //*p = 0x23;
    }
}
