#include "mm.h"
#include "bsp/registers/regsdigctl.h"
#include "config.h"
#include "lfs.h"
#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#undef assert
#define assert(x) 

#define CACHE (true)
#define BUFFER (true)

#define SECTION_DESC(SEG, ap, c, b, region) ((SEG << 20) | (ap << 10) | (region << 5) | (c << 3) | (b << 2) | (1 << 4) | (1 << 1))

#define ALIGN_UP(a, to) (((a) + ((to)-1)) & ~(((to)-1)))
#define ADDR_SECTION(x) (((uint32_t)x) & (0xFFF00000))
#define ADDR_SECTION_INDEX(x) (((uint32_t)x) >> 20)
#define ADDR_SECTION_OFFSET_PAGE(x) ((((uint32_t)x) & 0xFFFFF) / PAGE_SIZE)

typedef struct page_frame_attr_t {
    uint32_t map_addrs;
    unsigned mmap_socket : 4;
    unsigned dirty : 1;
    unsigned writeback : 1; 
    unsigned for_side_load : 1;
    unsigned lock : 1;

} page_frame_attr_t;

volatile uint32_t side_load_pending_off = -1;
volatile uint32_t side_load_pending_paddr = 0;

mmap_table_t *mmap_socket;
page_frame_attr_t *page_frame_attr;
int page_frame_ptr = 0;

extern int pgt_start;
extern int pgt_end;
extern int pgt_size;
extern int page_frame_start;
extern int page_frame_end;
extern int __BOOT1_HEAD_END;

uint32_t *l2pgt_mapaddr;

int mmaps = 0;
uint32_t memory_max;

uint32_t app_mem_warped_at = MAIN_MEMORY_BASE;
uint32_t app_memory_max = 0;

l1_fine_page_desc_t *L1PGT = (l1_fine_page_desc_t *)0x800C0000;
l2_tiny_page_desc_t *pgt = (l2_tiny_page_desc_t *)&pgt_start;

//#define PGT_BOOT1_MEM   (1)
#define PGT_MAIN_MEM   (0)

#define FILE_MAP_PGT_START  (1)

// 0:  BOOT1
// 1： MAIN MEMORY
// 2 3 4 ... NO USED
uint8_t *page_frames = (uint8_t *)&page_frame_start;

static inline void SetMPTELoc(uint32_t mpte, uint32_t seg) {
    BF_CS1n(DIGCTL_MPTEn_LOC, mpte, LOC, seg);
}

static inline uint32_t GetMPTELoc(uint32_t mpte) {
    return BF_RDn(DIGCTL_MPTEn_LOC, mpte, LOC);
}

void set_l2_desc(l2_tiny_page_desc_t *pgt, uint32_t entry,
                 uint32_t map_to_addr, uint8_t page_type, bool buffer, bool cache, uint8_t AP) {
    l2_tiny_page_desc_t a;
    a.map_to_addr = map_to_addr >> 10;
    a.page_type = page_type;
    a.buffer = buffer;
    a.cache = cache;
    a.AP = AP;
    a.Reserve = 0;
    pgt[entry] = a;
    //printf("set pgt:%08x\r\n", (uint32_t)&pgt[entry]);
    //mmu_clean_dcache(&pgt[entry], sizeof(l2_tiny_page_desc_t));
    // printf("SETL2:%08X\r\n",pgt[entry]);;
}

void set_l1_desc(uint32_t entry, uint32_t l2_ptg_entry, uint8_t domain, uint8_t type) {
    l1_fine_page_desc_t a;
    a.domain = domain;
    a.l2_pgt_base = l2_ptg_entry >> 12;
    a.type = type;
    a.sbo = 1;
    a.sbz0 = 0;
    a.sbz1 = 0;
    L1PGT[entry] = a;
    // printf("SETL1:%08X\r\n",L1PGT[entry]);;
}

void mm_init() {

    
    //mmu_invalidate_dcache_all();
    
    memory_max = ((uint32_t)&page_frame_size) - (MAX_RESERVE_FILE_PAGES * PAGE_SIZE);
    printf("memory max:%08lx\r\n", memory_max);
    printf("PAGE_FRAMES:%08lx\r\n", PAGE_FRAMES);
    l2pgt_mapaddr = calloc(MAX_ALLOW_MMAP_FILE + 1, sizeof(uint32_t));
    for (int i = 3; i < 8; i++) {
        SetMPTELoc(i, 0xEF3 + i);
        L1PGT[0xEF3 + i].type = SECTION_TYPE_FALSE;
    }
    mmap_socket = calloc(1, sizeof(mmap_table_t) * MAX_ALLOW_MMAP_FILE);
    page_frame_attr = calloc(1, sizeof(page_frame_attr_t) * PAGE_FRAMES);
    if ((!mmap_socket) || (!page_frame_attr)) {
        printf("mm_init ERROR\r\n");
        return;
    }
 
    memset(pgt, 0, (uint32_t)&pgt_size);

    for (int i = 0; i < PAGE_FRAMES; i++) {
        page_frame_attr[i].dirty = 0;
        page_frame_attr[i].lock = 0;
        page_frame_attr[i].writeback = 0;
        page_frame_attr[i].map_addrs = 0; 
        page_frame_attr[i].mmap_socket = -1;
    }

    //l2_tiny_page_desc_t *cur_pgt = (l2_tiny_page_desc_t *)&pgt[PGT_BOOT1_MEM * PGT_ITEMS];
    //for (uint32_t addr = 0; addr < 0x80000; addr += PAGE_SIZE) { //ALIGN_UP(PA(&page_frame_end), PAGE_SIZE)
    //    set_l2_desc(cur_pgt, addr / PAGE_SIZE, PA(addr), PAGE_TYPE_TINY, BUFFER, CACHE, AP_SYS_RW__USER_NONE);
    //}

    // mmu_clean_dcache(cur_pgt, ALIGN_UP(PA(&pgt_end), PAGE_SIZE));

    SetMPTELoc(2, ADDR_SECTION_INDEX(MAIN_MEMORY_BASE));
    set_l1_desc(ADDR_SECTION_INDEX(MAIN_MEMORY_BASE), PA(&pgt[PGT_MAIN_MEM * PGT_ITEMS]), 0, SECTION_TYPE_FINE);

    SetMPTELoc(1, ADDR_SECTION_INDEX(BOOT1_HEAD_LOAD_ADDR));
    ((volatile uint32_t *)L1PGT)[ADDR_SECTION_INDEX(BOOT1_HEAD_LOAD_ADDR)] = SECTION_DESC(0, AP_SYS_RW__USER_NONE, CACHE, BUFFER, 0);
    //set_l1_desc(ADDR_SECTION_INDEX(BOOT1_HEAD_LOAD_ADDR), PA(&pgt[PGT_BOOT1_MEM * PGT_ITEMS]), 0, SECTION_TYPE_FINE);

    
    ((volatile uint32_t *)L1PGT)[0] = SECTION_DESC(0, AP_SYS_RW__USER_NONE, false, false, 0);
    ((volatile uint32_t *)L1PGT)[0x800] = SECTION_DESC(0x800, AP_SYS_RW__USER_NONE, false, false, 0);
    // set_l2_desc(&pgt[1 * PGT_SIZE], ADDR_SECTION_OFFSET_PAGE(MAIN_MEMORY_BASE), PA(&page_frames[0 * PAGE_SIZE]), PAGE_TYPE_TINY, 1, 1, AP_SYS_RW__USER_RO);

    mmu_invalidate_tlb();

    // printf("alloc:%p\r\n",page_frame);
    // memset(mmap_page_frame, 0 ,sizeof(mmap_page_frame));
}

void munmap(uint32_t i) 
{
    INFO("c munmap:%ld\r\n", i);

    if (i >= MAX_ALLOW_MMAP_FILE) {
        
        INFO("c munmap ERR:%ld\r\n", i);
        return;
    }
    
    for(int j = 0; j< PAGE_FRAMES; j++)
    {
        clean_file_page(j);
    }

    for(int j = 0; j< PAGE_FRAMES; j++)
    {
        if(page_frame_attr[j].mmap_socket == i)
        {
            
            //mmu_invalidate_icache();
            if (L1PGT[ADDR_SECTION_INDEX(page_frame_attr[j].map_addrs)].type != SECTION_TYPE_FALSE) {
                uint32_t *p = (uint32_t *)PA(L1PGT[ADDR_SECTION_INDEX(page_frame_attr[j].map_addrs)].l2_pgt_base << 12);
                p += ADDR_SECTION_OFFSET_PAGE(page_frame_attr[j].map_addrs);
                *p = 0;
                //L1PGT[ADDR_SECTION_INDEX(page_frame_attr[page_frame_ptr].map_addrs)].type = SECTION_TYPE_FALSE;
                //SetMPTELoc(ADDR_SECTION_INDEX(page_frame_attr[page_frame_ptr].map_addrs), 0xEDF+ADDR_SECTION_INDEX(page_frame_attr[page_frame_ptr].map_addrs));
            }

            page_frame_attr[j].map_addrs = 0;

            //memset((void *)(&page_frames[j * PAGE_SIZE]), 0xEA, PAGE_SIZE);
            //page_frame_attr[j].lock = 0;
        }
    }

    if (mmap_socket[i].path) {
        if(mmap_socket[i].path[0] != 3)
        {

            
            lfs_file_close(&lfs, mmap_socket[i].f);
            free(mmap_socket[i].f);

            for (int k = 0; k < MAX_ALLOW_MMAP_FILE; k++) { 
                if((mmap_socket[k].path) && (k != i))
                    if(memcmp(mmap_socket[k].path, mmap_socket[i].path, strlen(mmap_socket[k].path)) == 0)
                    {
                        mmap_socket[k].path = NULL;
                        mmap_socket[k].f = NULL;
                        printf("munmap same file:%ld %d\r\n", i, k);
                    }
            }

        }
        free(mmap_socket[i].path);
    
        mmap_socket[i].path = NULL;
        mmap_socket[i].f = NULL;
 
        mmu_invalidate_icache();
        mmu_invalidate_dcache((void *)mmap_socket[i].map_to_address, mmap_socket[i].size);
        mmu_invalidate_tlb();

        mmap_socket[i].map_to_address = 0;
    }
}
 
 

int mmap(uint32_t to_addr, const char *path, uint32_t offset, uint32_t size, bool writable, bool writeback) {
    int mmap_i = 0;
    uint32_t pathLen = strlen(path);
    char *p = calloc(pathLen + 1, 1);

    if (!p) {
        
        printf("mmap No mem1\r\n");
        return -ENOMEM;
    } 

    for (mmap_i = 0; mmap_i < MAX_ALLOW_MMAP_FILE; mmap_i++) {
        if (!mmap_socket[mmap_i].path) {
            goto found;
        }
    }
    printf("mmap No slot\r\n");
    return -ENOSPC;

found:

    strcpy(p , path);
    
    mmap_socket[mmap_i].path = p;
    mmap_socket[mmap_i].map_to_address = to_addr;
    mmap_socket[mmap_i].file_offset = offset;
//    mmap_socket[mmap_i].pgt = (l2_tiny_page_desc_t *)&pgt[(FILE_MAP_PGT_START + mmap_i) * PGT_ITEMS];
    mmap_socket[mmap_i].writable = writable;
    mmap_socket[mmap_i].writeback = writeback;
    
    if(p[0] == 3)
    {
        mmap_socket[mmap_i].f = NULL;
    }else{

        for (int i = 0; i < MAX_ALLOW_MMAP_FILE; i++) { 
            if((mmap_socket[i].path) && (i != mmap_i))
                if(memcmp(mmap_socket[i].path, mmap_socket[mmap_i].path, pathLen) == 0)
                {
                    mmap_socket[mmap_i].f = mmap_socket[i].f;
                    printf("same file:%d %d\r\n", i, mmap_i);
                    goto after_open_file;
                }
        }

        mmap_socket[mmap_i].f = calloc(1, sizeof(lfs_file_t)); 
        if (!mmap_socket[mmap_i].f) {
            free(p); 
            mmap_socket[mmap_i].path = NULL;
            mmap_socket[mmap_i].f = NULL;
            mmap_socket[mmap_i].map_to_address = 0;
            printf("mmap No mem2\r\n");
            return -ENOMEM;
        }
    }

    int err = -1;
    if(p[0] == 3)
    {
        err = 0;
    }else{
        printf("mmap:open [%s, %s]  \r\n", mmap_socket[mmap_i].path, p);
        err = lfs_file_open(&lfs, mmap_socket[mmap_i].f, mmap_socket[mmap_i].path, mmap_socket[mmap_i].writeback ? LFS_O_RDWR : LFS_O_RDONLY);
    }
    if (err != LFS_ERR_OK) {
        free(p);
        free(mmap_socket[mmap_i].f);
            mmap_socket[mmap_i].map_to_address = 0;
            mmap_socket[mmap_i].path = NULL;
            mmap_socket[mmap_i].f = NULL;
        printf("FOPEN ERR:%d\r\n", err);
        return err;
    }

after_open_file:
    if (size) {
        mmap_socket[mmap_i].size = size;
    } else {
        if(p[0] != 3)
            mmap_socket[mmap_i].size = lfs_file_size(&lfs, mmap_socket[mmap_i].f) - mmap_socket[mmap_i].file_offset;
        else{
            mmap_socket[mmap_i].size = 0;
        }
    }
    printf("f:%s,fsz:%ld\r\n", p, mmap_socket[mmap_i].size);
    memset(&pgt[(FILE_MAP_PGT_START + mmap_i) * PGT_ITEMS], 0, PGT_SIZE);
    mmu_clean_dcache(&pgt[(FILE_MAP_PGT_START + mmap_i) * PGT_ITEMS], PGT_SIZE);
    return mmap_i;
}

void clean_file_page(uint32_t page) {
    if (page_frame_attr[page].dirty) {
        mmap_table_t *m = &mmap_socket[page_frame_attr[page].mmap_socket];
        if (m->writable && m->writeback) {
            lfs_off_t file_offset = page_frame_attr[page].map_addrs - (uint32_t)m->map_to_address + m->file_offset;
            // lfs_soff_t toff =
            mmu_clean_dcache((void *)page_frame_attr[page].map_addrs, PAGE_SIZE);
            lfs_file_seek(&lfs, m->f, file_offset, LFS_SEEK_SET);
            // lfs_ssize_t sz =
            lfs_file_write(&lfs, m->f, (const void *)PA(&page_frames[page * PAGE_SIZE]), PAGE_SIZE);
            // printf("WB:ofA:%ld ,ofB: %ld,sz: %ld\r\n", file_offset, toff, sz);
            page_frame_attr[page].dirty = 0;

            // memset((void *)PA(&page_frames[page * PAGE_SIZE]), 0, PAGE_SIZE);
        }
    }
}

void mm_set_app_mem_warp(uint32_t svaddr, uint32_t pages)
{
    if(svaddr == 0)
    {
        app_mem_warped_at = MAIN_MEMORY_BASE;
        app_memory_max = 0;
        side_load_pending_off = 0;
        side_load_pending_paddr = 0;
        return;
    }

    app_mem_warped_at = svaddr;
    app_memory_max = pages * PAGE_SIZE;


    for (int i = 0; i < PAGE_FRAMES; i++) {
        if ((page_frame_attr[i].map_addrs >= app_mem_warped_at) && 
                (page_frame_attr[i].map_addrs < (app_mem_warped_at + app_memory_max))) { 
                if (page_frame_attr[i].dirty) {
                    page_frame_attr[i].dirty = 0;
                    if (page_frame_attr[i].map_addrs) {
                        if (L1PGT[ADDR_SECTION_INDEX(page_frame_attr[i].map_addrs)].type != SECTION_TYPE_FALSE) {
                            uint32_t *p = (uint32_t *)PA(L1PGT[ADDR_SECTION_INDEX(page_frame_attr[i].map_addrs)].l2_pgt_base << 12);
                            p += ADDR_SECTION_OFFSET_PAGE(page_frame_attr[i].map_addrs);
                            *p = 0;
                        }
                        mmu_invalidate_dcache((void *)page_frame_attr[i].map_addrs, PAGE_SIZE);
                        page_frame_attr[i].map_addrs = 0;
                    }
                    memset((void *)(&page_frames[i * PAGE_SIZE]), 0xEA, PAGE_SIZE);
            }
        }
    }
    
    mmu_invalidate_tlb();

    INFO("mm warp:%08lx,%ld\r\n",svaddr,pages);
}

uint32_t mm_trim_page(uint32_t vaddr) {
    uint32_t trim_pages = 0;
    for (int i = 0; i < PAGE_FRAMES; i++) {
        //printf("find: %08lX %08lX\r\n",  page_frame_attr[i].map_addrs,          (vaddr & ~(PAGE_SIZE - 1)    ) );
        if (       (page_frame_attr[i].map_addrs & (~(PAGE_SIZE - 1))) == (vaddr & (~(PAGE_SIZE - 1)))      ) {
            //if (page_frame_attr[i].dirty)  
            {
                trim_pages = 1;
                page_frame_attr[i].dirty = 0;
                mmu_invalidate_dcache((void *)page_frame_attr[i].map_addrs, PAGE_SIZE);
                if (L1PGT[ADDR_SECTION_INDEX(page_frame_attr[i].map_addrs)].type != SECTION_TYPE_FALSE) {
                    uint32_t *p = (uint32_t *)PA(L1PGT[ADDR_SECTION_INDEX(page_frame_attr[i].map_addrs)].l2_pgt_base << 12);
                    p += ADDR_SECTION_OFFSET_PAGE(page_frame_attr[i].map_addrs);
                    *p = 0;
                    mmu_invalidate_tlb();
                }
                page_frame_attr[i].map_addrs = 0;
                
                memset((void *)(&page_frames[i * PAGE_SIZE]), 0xEF, PAGE_SIZE);
            }
            break;
        }
    }
    return trim_pages;
}

uint32_t mm_lock_vaddr(uint32_t vaddr, uint32_t lock)
{
    for (int i = 0; i < PAGE_FRAMES; i++) {
         if (page_frame_attr[i].map_addrs == (vaddr & (~(PAGE_SIZE - 1)))) 
         {
            page_frame_attr[i].lock = lock;
            return 0;
         }
    }
    return -1;
}

void dumpMpte()
{
    for(int i = 0; i < 8; i++)
    {
        printf("============MPTE %d LOC:%03lX\r\n",i, GetMPTELoc(i));
    }
}

uint32_t get_l1_mpte(uint32_t faddr)
{
    static uint32_t curSel = 2;
    for(int i = 3; i < 8; i ++)
    {
        if(GetMPTELoc(i) ==  ADDR_SECTION_INDEX(faddr))
        {
            return i;
        }
    }

    curSel++;
    if(curSel > 7)
    {   
        curSel = 3;
    }


    SetMPTELoc(curSel, ADDR_SECTION_INDEX(faddr));
    memset((void *)PA(&pgt[(curSel-2) * PGT_ITEMS]), 0, PGT_SIZE);

    return curSel;
}


uint32_t get_new_page_frame(uint32_t faddr) {
    uint32_t cnt = 0;
    int get_ptr;
    bool not_found = 0;

    for(int i = 0; i < PAGE_FRAMES; i++)
    {
        if(page_frame_attr[i].map_addrs == (faddr & ~(PAGE_SIZE - 1)))
        {
            if(!page_frame_attr[i].dirty)
            {
                return i;
            }else{
                break;
            }
        }
    }

    while ((page_frame_attr[page_frame_ptr].dirty) || (page_frame_attr[page_frame_ptr].lock)) {
        page_frame_ptr++;
        if (page_frame_ptr >= PAGE_FRAMES) {
            page_frame_ptr = 0;
        }
        cnt++;
        if (cnt >= PAGE_FRAMES) {
            not_found = 1;
            break;
        }
    }
    if (not_found) {
        cnt = 0;
        while (((page_frame_attr[page_frame_ptr].mmap_socket) == 0b1111) || (page_frame_attr[page_frame_ptr].lock) ) {
            page_frame_ptr++;
            cnt++;
            if (page_frame_ptr >= PAGE_FRAMES) {
                page_frame_ptr = 0;
            }
            if (cnt >= PAGE_FRAMES) {
                printf("ERROR: no free page!\r\n");
                while (1)
                    ;
            }
        }
        // printf("Start Clean\r\n");
        // clean_file_page(page_frame_ptr);
        for (int i = 0; i < PAGE_FRAMES; i++) {
            clean_file_page(i);
        }
        // printf("END Clean\r\n");
        //
    }
    assert(!page_frame_attr[page_frame_ptr].dirty);
    assert(!page_frame_attr[page_frame_ptr].lock);

    if (page_frame_attr[page_frame_ptr].map_addrs) {
        mmu_invalidate_dcache((void *)page_frame_attr[page_frame_ptr].map_addrs, PAGE_SIZE);
        //mmu_invalidate_icache();
        if (L1PGT[ADDR_SECTION_INDEX(page_frame_attr[page_frame_ptr].map_addrs)].type != SECTION_TYPE_FALSE) {
            uint32_t *p = (uint32_t *)PA(L1PGT[ADDR_SECTION_INDEX(page_frame_attr[page_frame_ptr].map_addrs)].l2_pgt_base << 12);
            p += ADDR_SECTION_OFFSET_PAGE(page_frame_attr[page_frame_ptr].map_addrs);
            *p = 0;
            //L1PGT[ADDR_SECTION_INDEX(page_frame_attr[page_frame_ptr].map_addrs)].type = SECTION_TYPE_FALSE;
            //SetMPTELoc(ADDR_SECTION_INDEX(page_frame_attr[page_frame_ptr].map_addrs), 0xEDF+ADDR_SECTION_INDEX(page_frame_attr[page_frame_ptr].map_addrs));
        }
    }
    get_ptr = page_frame_ptr;
    page_frame_ptr++;
    if (page_frame_ptr >= PAGE_FRAMES) {
        page_frame_ptr = 0;
    }
    return get_ptr;
}

int set_new_pgf(uint32_t faddr, int ___x) {
    int pgtid;
    int pgf = get_new_page_frame(faddr);
    //printf("pgtid:%d, get pgf:%d to %ld, faddr:%08lX\r\n",pgtid, pgf, ADDR_SECTION_OFFSET_PAGE(faddr), faddr);
    //assert(PA(&page_frames[pgf * PAGE_SIZE]) >= 0x16000);
    //assert(PA(&page_frames[pgf * PAGE_SIZE]) < 0x80000);
    //int mpet = 0;
    

    if(___x == PGT_MAIN_MEM)
    {
        pgtid = 0;
    }else
    {
        //mpet = 
        pgtid = get_l1_mpte(faddr) - 2;

        //pgtid = get_l2_pgt(faddr);
    }

    
    //l2pgt_mapaddr[pgtid] = ADDR_SECTION_INDEX(faddr);

    set_l1_desc(ADDR_SECTION_INDEX(faddr), PA(&pgt[(pgtid) * PGT_ITEMS]), 0, SECTION_TYPE_FINE);
    
    //printf("set l1pte:%08X\r\n", L1PGT[ADDR_SECTION_INDEX(faddr)].l2_pgt_base << 12);
    
    //printf("get-l2pgt:%d, mpte:%d, pgf:%d, faddr:%08lX SETL1:%08X\r\n", pgtid, mpet, pgf, faddr, L1PGT[ADDR_SECTION_INDEX(faddr)].l2_pgt_base << 12);


    set_l2_desc((&pgt[pgtid * PGT_ITEMS]),
                ADDR_SECTION_OFFSET_PAGE(faddr),
                PA(&page_frames[pgf * PAGE_SIZE]),
                PAGE_TYPE_TINY, BUFFER, CACHE, AP_SYS_RO__USER_RO);
    
    mmu_clean_dcache(&pgt[pgtid * PGT_ITEMS], PGT_SIZE);

    page_frame_attr[pgf].map_addrs = (faddr & ~(PAGE_SIZE - 1));
    //mmu_drain_buffer();
    mmu_invalidate_tlb();
    return pgf;
}

void set_pgf_perm(uint32_t faddr) {
    for (int i = 0; i < PAGE_FRAMES; i++) {
        if (page_frame_attr[i].map_addrs == (faddr & ~(PAGE_SIZE - 1))) {
            //printf("set dirty:%d\r\n", i);
            page_frame_attr[i].dirty = 1;
            break;
        }
    }
    l2_tiny_page_desc_t *c = (l2_tiny_page_desc_t *)VA((L1PGT[ADDR_SECTION_INDEX(faddr)].l2_pgt_base << 12));
    assert(c);
    c += ADDR_SECTION_OFFSET_PAGE(faddr);
    c->AP = AP_SYS_RW__USER_RW;
    mmu_clean_dcache(c, sizeof(l2_tiny_page_desc_t));
    //mmu_drain_buffer();
    mmu_invalidate_tlb();
}


uint32_t mm_vaddr_map_pa(uint32_t vaddr)
{ 
    uint32_t found = 0;
    int cnt = 0;
    for(int i = 0; i < PAGE_FRAMES;i ++)
    {
        if (page_frame_attr[i].map_addrs == (vaddr & ~(PAGE_SIZE - 1))) {
            found = PA(&page_frames[i * PAGE_SIZE]);
            cnt ++;
            //printf("Search :%08lX, Found:%08lX\r\n", vaddr, found);
            //return PA(&page_frames[i * PAGE_SIZE]);
        }
    }
    if(cnt >= 2)
    {
        INFO("WARN:!!! Found %d Same Pages!\r\n", cnt);
    }
    return found;
}

#define IF_BETWEEN(x, a, b) if (((x) >= (a)) && ((x) < ((a) + (b))))
#define BETWEEN(x, a, b) ((x) >= (a)) && ((x) < (b))


uint32_t is_addr_vaild(uint32_t vaddr, bool writable) {
    //printf("chk addr:%08lx\r\n",vaddr);
    if(
    ((vaddr >= MAIN_MEMORY_BASE )
    && (vaddr < app_mem_warped_at))
    || 
    ((vaddr >= (app_mem_warped_at + app_memory_max))
    && (vaddr < MAIN_MEMORY_BASE + memory_max))
    )
    {
        return 1;
    }

    IF_BETWEEN(vaddr, MAIN_MEMORY_BASE, memory_max) {
        return 1;
    }

    IF_BETWEEN(vaddr, APP_MEMORY_BASE, app_memory_max) { 
        return 1;
    }

    for (int i = 0; i < MAX_ALLOW_MMAP_FILE; i++) {
        IF_BETWEEN(vaddr, (uint32_t)mmap_socket[i].map_to_address, mmap_socket[i].size) {
            if(mmap_socket[i].writable >= writable)
                return 1;
        }
    }

    return 0;
}
 

int do_dab(uint8_t FSR, uint32_t faddr, uint32_t pc, uint8_t faultMode) {
    switch (FSR) {
    case 0b101: // 5  // Translation
    case 0b111: // 7
    case 0xF5:  // DAB
        if(
            ((faddr >= MAIN_MEMORY_BASE )
            && (faddr < app_mem_warped_at))
            || 
            ((faddr >= (app_mem_warped_at + app_memory_max))
            && (faddr < MAIN_MEMORY_BASE + memory_max))
            )
        {
            int pgf = set_new_pgf(faddr, PGT_MAIN_MEM);
            page_frame_attr[pgf].mmap_socket = 0b1111;
            return 0;
        }else IF_BETWEEN(faddr, APP_MEMORY_BASE, app_memory_max) {
            int pgf = set_new_pgf(faddr, PGT_MAIN_MEM);
            page_frame_attr[pgf].mmap_socket = 0b1111;
            return 0;
        }else {
            for (int i = 0; i < MAX_ALLOW_MMAP_FILE; i++) {
                IF_BETWEEN(faddr, (uint32_t)mmap_socket[i].map_to_address, mmap_socket[i].size) {
                    // assert(i != 0b111);
                    //if (GetMPTELoc(3 + i) != ADDR_SECTION_INDEX(faddr)) {
                    //    memset(&pgt[(FILE_MAP_PGT_START + i) * PGT_ITEMS], 0, PGT_SIZE);
                    //    //memset(&pgt[(FILE_MAP_PGT_START + i) * PGT_ITEMS], 0, PGT_SIZE);
                    //}
                    //SetMPTELoc(3 + i, ADDR_SECTION_INDEX(faddr));
                    //set_l1_desc(ADDR_SECTION_INDEX(faddr), PA(&pgt[(FILE_MAP_PGT_START + i) * PGT_ITEMS]), 0, SECTION_TYPE_FINE);
                    
                    int pgf = set_new_pgf(faddr, FILE_MAP_PGT_START + i);
                    page_frame_attr[pgf].mmap_socket = i & 0b1111; 
                    
                    // page_frame_attr[pgf].writeback = mmap_socket[i].writeback;
                    if(mmap_socket[i].path[0] == '\03')
                    {
                        side_load_pending_paddr = PA(&page_frames[pgf * PAGE_SIZE]);
                        side_load_pending_off = (faddr & ~(PAGE_SIZE - 1)) - (uint32_t)mmap_socket[i].map_to_address + mmap_socket[i].file_offset;
                        //printf("F:%08lX,  %08lX, ms:%08lX\r\n",faddr, side_load_pending_paddr, side_load_pending_off );
                        //page_frame_attr[pgf].for_side_load = 1;
                        page_frame_attr[pgf].lock = 1;
                        return 1;
                    }else{
                        lfs_off_t file_offset = (faddr & ~(PAGE_SIZE - 1)) - (uint32_t)mmap_socket[i].map_to_address + mmap_socket[i].file_offset;
                        lfs_file_seek(&lfs, mmap_socket[i].f, file_offset, LFS_SEEK_SET);
                        // lfs_ssize_t rdc =
                        lfs_file_read(&lfs, mmap_socket[i].f, (void *)PA(&page_frames[pgf * PAGE_SIZE]), PAGE_SIZE);
                        //lfs_file_read(&lfs, mmap_socket[i].f, (void *)(faddr & ~(PAGE_SIZE - 1)), PAGE_SIZE);
                        // assert(rdc > 0);
                        // printf("rd fo:%ld,rd:%ld\r\n", file_offset, rdc);
                    }

                
                    //mmu_clean_dcache((void *)(&page_frames[pgf * PAGE_SIZE]), PAGE_SIZE);
                    //mmu_drain_buffer();
                    mmu_invalidate_dcache((void *)(faddr & ~(PAGE_SIZE - 1)), PAGE_SIZE);
                    if (FSR == 0xF5) 
                    {
                        mmu_invalidate_icache();
                    }

                    return 0;
                }
            }
            goto err;
        }
        goto err;

    case 0b1111: // F
    case 0b1101: // D Permission
        if(
            ((faddr >= MAIN_MEMORY_BASE )
            && (faddr < app_mem_warped_at))
            || 
            ((faddr >= (app_mem_warped_at + app_memory_max))
            && (faddr < MAIN_MEMORY_BASE + memory_max))
            )
        {
            if(faultMode == USER_MODE)
            {
                printf("USER_MODE FAULT\r\n");
                return -1;
            }
            set_pgf_perm(faddr);
            return 0;
        }else IF_BETWEEN(faddr, APP_MEMORY_BASE, app_memory_max) {
            set_pgf_perm(faddr);
            return 0;
        }else {
            for (int i = 0; i < MAX_ALLOW_MMAP_FILE; i++) {
                IF_BETWEEN(faddr, (uint32_t)mmap_socket[i].map_to_address, mmap_socket[i].size) {
                    if (mmap_socket[i].writable == false) {
                        INFO("mmap:%d can't write.\r\n", i);
                        goto err;
                    } else {
                        set_pgf_perm(faddr);
                        return 0;
                    }
                }
            }
        }

        goto err;
    default:
        goto err;
    }

    return -1;

err:

    printf("ERROR:%08lX, %08lX, FSR:%02X\r\n", faddr, pc, FSR);

    return -1;
    // while (1) ;
}
