#pragma once


#include <stdint.h>
#include <stdbool.h>
#include "sys_mmap.h"


typedef struct mmap_info
{
    uint32_t map_to;
    const char *path;
    uint32_t offset;
    uint32_t size;
    bool writable;
    bool writeback;
}mmap_info;

#define LL_SWI_BASE                     (0xEE00)
#define LL_SWI_SET_IRQ_VECTOR           (LL_SWI_BASE + 8)
#define LL_SWI_SET_SVC_VECTOR           (LL_SWI_BASE + 13)

#define LL_SWI_SET_MEM_TRIM             (LL_SWI_BASE + 100)
#define LL_SWI_MEM_IS_VAILD             (LL_SWI_BASE + 101)
#define LL_SWI_SET_DAB_VECTOR            (LL_SWI_BASE + 102)
#define LL_SWI_SET_PAB_VECTOR            (LL_SWI_BASE + 103)
#define LL_SWI_SET_APP_MEM_WARP          (LL_SWI_BASE + 104)
#define LL_SWI_GET_PENDING_PGFOFF          (LL_SWI_BASE + 105)
#define LL_SWI_GET_PENDING_PGADDR         (LL_SWI_BASE + 106)
#define LL_SWI_MMAP             (LL_SWI_BASE + 107)
#define LL_SWI_MUNMAP           (LL_SWI_BASE + 108)

#define LL_SWI_SET_PAB_STACK           (LL_SWI_BASE + 109)
#define LL_SWI_SET_DAB_STACK           (LL_SWI_BASE + 110)
#define LL_SWI_GET_VADDR_PA            (LL_SWI_BASE + 111)
#define LL_SWI_GET_VADDR_LOCK          (LL_SWI_BASE + 112)
#define LL_SWI_SET_SVC_STACK           (LL_SWI_BASE + 113)
#define LL_SWI_EXIT_SYS_SVC            (LL_SWI_BASE + 114)



#define LL_SWI_FS_SIZE                 (LL_SWI_BASE + 119)
#define LL_SWI_FS_REMOVE               (LL_SWI_BASE + 120)
#define LL_SWI_FS_RENAME               (LL_SWI_BASE + 121)
#define LL_SWI_FS_STAT                 (LL_SWI_BASE + 122)
#define LL_SWI_FS_OPEN                 (LL_SWI_BASE + 123)
#define LL_SWI_FS_CLOSE                (LL_SWI_BASE + 124)
#define LL_SWI_FS_SYNC                 (LL_SWI_BASE + 125)
#define LL_SWI_FS_READ                 (LL_SWI_BASE + 126)
#define LL_SWI_FS_WRITE                (LL_SWI_BASE + 127)
#define LL_SWI_FS_SEEK                 (LL_SWI_BASE + 128)
#define LL_SWI_FS_REWIND               (LL_SWI_BASE + 129)
#define LL_SWI_FS_TRUNCATE             (LL_SWI_BASE + 130)
#define LL_SWI_FS_TELL                 (LL_SWI_BASE + 131)

#define LL_SWI_FS_DIR_MKDIR               (LL_SWI_BASE + 132)
#define LL_SWI_FS_DIR_OPEN                (LL_SWI_BASE + 133)
#define LL_SWI_FS_DIR_CLOSE               (LL_SWI_BASE + 134)
#define LL_SWI_FS_DIR_SEEK                (LL_SWI_BASE + 135)
#define LL_SWI_FS_DIR_TELL                (LL_SWI_BASE + 136)
#define LL_SWI_FS_DIR_REWIND              (LL_SWI_BASE + 137)
#define LL_SWI_FS_DIR_READ                (LL_SWI_BASE + 138)
#define LL_SWI_FS_DIR_GET_CUR_TYPE                (LL_SWI_BASE + 139)
#define LL_SWI_FS_DIR_GET_CUR_NAME                (LL_SWI_BASE + 140)
#define LL_SWI_FS_DIR_GET_CUR_SIZE                (LL_SWI_BASE + 141)

#define LL_SWI_FS_GET_FOBJ_SZ                  (LL_SWI_BASE + 142)
#define LL_SWI_FS_GET_DIROBJ_SZ                (LL_SWI_BASE + 143)

typedef void* fs_obj_t;
typedef void* fs_dir_obj_t;

#define HCATTR   __attribute__((naked)) __attribute__((target("arm")))


#define DECDEF_LLSWI(ret, name, pars, SWINum)   \
    ret HCATTR name pars;



DECDEF_LLSWI(void,        ll_set_irq_vector,        (uint32_t addr)                             ,LL_SWI_SET_IRQ_VECTOR        );
DECDEF_LLSWI(void,        ll_set_svc_vector,        (uint32_t addr)                             ,LL_SWI_SET_SVC_VECTOR        );
           
DECDEF_LLSWI(void,        ll_set_dab_vector,        (uint32_t addr)                             ,LL_SWI_SET_DAB_VECTOR        );
DECDEF_LLSWI(void,        ll_set_pab_vector,        (uint32_t addr)                             ,LL_SWI_SET_PAB_VECTOR        );
DECDEF_LLSWI(void,        ll_set_dab_stack,         (uint32_t addr)                             ,LL_SWI_SET_DAB_STACK         );
DECDEF_LLSWI(void,        ll_set_pab_stack,         (uint32_t addr)                             ,LL_SWI_SET_PAB_STACK         );
DECDEF_LLSWI(void,        ll_set_svc_stack,         (uint32_t addr)                             ,LL_SWI_SET_SVC_STACK         );
DECDEF_LLSWI(void,        ll_exit_svc,              (void)                                      ,LL_SWI_EXIT_SYS_SVC          );
    
DECDEF_LLSWI(uint32_t,    ll_mm_trim_vaddr,         (uint32_t addr)                             ,LL_SWI_SET_MEM_TRIM          );
DECDEF_LLSWI(uint32_t,    ll_vaddr_vaild,           (uint32_t addr)                             ,LL_SWI_MEM_IS_VAILD          );
DECDEF_LLSWI(void,        ll_set_app_mem_warp,      (uint32_t vaddr, uint32_t pages)            ,LL_SWI_SET_APP_MEM_WARP      );
DECDEF_LLSWI(int,         ll_mmap,                  (mmap_info *info)                           ,LL_SWI_MMAP                  );
DECDEF_LLSWI(void,        ll_mumap,                 (int map)                                   ,LL_SWI_MUNMAP                );
DECDEF_LLSWI(uint32_t,    ll_get_pending_pgpaddr,   (void)                                      ,LL_SWI_GET_PENDING_PGADDR    );
DECDEF_LLSWI(uint32_t,    ll_get_pending_pgfoff,    (void)                                      ,LL_SWI_GET_PENDING_PGFOFF    );
DECDEF_LLSWI(uint32_t,    ll_vaddr2paddr,           (uint32_t vaddr)                            ,LL_SWI_GET_VADDR_PA          );
DECDEF_LLSWI(uint32_t,    ll_vaddr_lock,            (uint32_t vaddr ,uint32_t lock)             ,LL_SWI_GET_VADDR_LOCK        );



DECDEF_LLSWI(uint32_t,    ll_fs_get_dirobj_sz,      (void)                                      ,LL_SWI_FS_GET_DIROBJ_SZ      );
DECDEF_LLSWI(uint32_t,    ll_fs_get_fobj_sz,        (void)                                      ,LL_SWI_FS_GET_FOBJ_SZ        );
DECDEF_LLSWI(int,         ll_fs_size,               (fs_obj_t fobj)                             ,LL_SWI_FS_SIZE               );
DECDEF_LLSWI(int,         ll_fs_remove,             (const char *path)                          ,LL_SWI_FS_REMOVE             );
DECDEF_LLSWI(int,         ll_fs_rename,             (const char *oldpath, const char *newpath)  ,LL_SWI_FS_RENAME             );
DECDEF_LLSWI(int,         ll_fs_open,               (fs_obj_t fobj, const char *path, int flag) ,LL_SWI_FS_OPEN               );
DECDEF_LLSWI(int,         ll_fs_close,              (fs_obj_t fobj)                             ,LL_SWI_FS_CLOSE              );
DECDEF_LLSWI(int,         ll_fs_sync,               (fs_obj_t fobj)                             ,LL_SWI_FS_SYNC               );
DECDEF_LLSWI(int,         ll_fs_read,               (fs_obj_t fobj, void* buf, uint32_t size)   ,LL_SWI_FS_READ               );
DECDEF_LLSWI(int,         ll_fs_write,              (fs_obj_t fobj, void* buf, uint32_t size)   ,LL_SWI_FS_WRITE              );
DECDEF_LLSWI(int,         ll_fs_seek,               (fs_obj_t fobj, uint32_t off, int whence)   ,LL_SWI_FS_SEEK               );
DECDEF_LLSWI(int,         ll_fs_rewind,             (fs_obj_t fobj)                             ,LL_SWI_FS_REWIND             );
DECDEF_LLSWI(int,         ll_fs_truncate,           (fs_obj_t fobj, uint32_t size)              ,LL_SWI_FS_TRUNCATE           );
DECDEF_LLSWI(uint32_t,    ll_fs_tell,               (fs_obj_t fobj)                             ,LL_SWI_FS_TELL               );
DECDEF_LLSWI(int,         ll_fs_dir_mkdir,          (const char* path)                          ,LL_SWI_FS_DIR_MKDIR          );
DECDEF_LLSWI(int,         ll_fs_dir_open,           (fs_dir_obj_t dir_obj, const char* path)    ,LL_SWI_FS_DIR_OPEN           );
DECDEF_LLSWI(int,         ll_fs_dir_close,          (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_CLOSE          );
DECDEF_LLSWI(int,         ll_fs_dir_seek,           (fs_dir_obj_t dir_obj, uint32_t off)        ,LL_SWI_FS_DIR_SEEK           );
DECDEF_LLSWI(uint32_t,    ll_fs_dir_tell,           (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_TELL           );
DECDEF_LLSWI(uint32_t,    ll_fs_dir_rewind,         (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_REWIND         );
DECDEF_LLSWI(uint32_t,    ll_fs_dir_read,           (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_READ           );
DECDEF_LLSWI(const char *,ll_fs_dir_cur_item_name,  (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_GET_CUR_NAME   );
DECDEF_LLSWI(uint32_t,    ll_fs_dir_cur_item_size,  (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_GET_CUR_SIZE   );
DECDEF_LLSWI(int,         ll_fs_dir_cur_item_type,  (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_GET_CUR_TYPE   );

