#include "hypercall.h"



#undef DECDEF_LLSWI
#define DECDEF_LLSWI(ret, name, pars, SWINum)                              \
    ret HCATTR name pars                                                 \
    {                                                                   \
        __asm volatile("swi %0" :: "i"(SWINum));                        \
        __asm volatile("bx lr");                                        \
    }




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


