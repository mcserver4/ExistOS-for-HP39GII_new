
#include "mm.h"
#include "utils.h"
#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include "config.h"
// unsigned int faultAddress;
// volatile unsigned int insAddress;
// unsigned int FSR;

//uint32_t sys_irq_base = 0;
//uint32_t sys_svc_base = 0;

volatile uint32_t in_swi = 0;
volatile uint32_t in_irq = 0;
 
void (*sys_irq_base)( ) = (void *)NULL;
void (*sys_svc_base)( ) = (void *)NULL;
void (*sys_dab_base)( ) = (void *)NULL;
void (*sys_pab_base)( ) = (void *)NULL;

volatile uint32_t dab_context[17 * 4]; //CPSR R0 - R15
volatile uint32_t *dab_context_ptr = dab_context;
volatile uint32_t pab_context[17 * 4]; //CPSR R0 - R15
volatile uint32_t *pab_context_ptr = pab_context;
volatile uint32_t svc_context[17 * 7]; //CPSR R0 - R15
volatile uint32_t *svc_context_ptr = svc_context; 
volatile uint32_t und_context[17 * 3]; //CPSR R0 - R15
volatile uint32_t *und_context_ptr = und_context;

uint32_t* __attribute__((target("arm"))) __attribute__((naked)) get_sp( ) {
    __asm volatile("mov r0,sp");
    __asm volatile("bx lr");
}

uint32_t __attribute__((target("arm"))) __attribute__((naked)) get_spsr( ) {
    __asm volatile("mrs r0,spsr");
    __asm volatile("bx lr");
}

uint32_t __attribute__((target("arm"))) __attribute__((naked)) getFaddr()
{
    register uint32_t faultAddress asm("r0");
    __asm volatile("mrc p15, 0, %0, c6, c0, 0"
                   :
                   : "r"(faultAddress));
    __asm volatile("bx lr");
}

uint32_t __attribute__((target("arm"))) __attribute__((naked)) getFsr()
{
    register uint32_t FSR asm("r0");
    __asm volatile("mrc p15, 0, %0, c5, c0, 0"
                   :
                   : "r"(FSR));
    __asm volatile("bx lr");
}

uint8_t __attribute__((target("arm"))) __attribute__((naked)) get_cur_mode(void) {
    __asm volatile("mrs r0,cpsr_all");
    __asm volatile("and r0,r0,#0x1F");
    __asm volatile("bx lr");
}

uint8_t __attribute__((target("arm"))) __attribute__((naked)) get_saved_mode(void) {
    __asm volatile("mrs r0,spsr_all");
    __asm volatile("and r0,r0,#0x1F");
    __asm volatile("bx lr");
}

void volatile __attribute__((target("arm"))) __attribute__((naked)) arm_vector_und() {
    
    __asm volatile("stmdb sp!, {r0-r12, lr}"); 
    print_uart0("1!!und\r\n");
    uint32_t *cur_sp = get_sp();
    for(int i = 0; i < 14; i++)
    {
        printf("%d:%08lX\r\n",i, cur_sp[i]);
    }
    printf("AT:%08lX: %08lX\r\n", cur_sp[13], *((uint32_t *)cur_sp[13])  );


    for(int i = 0; i <=  12; i++)
    {
        und_context_ptr[i+1] = cur_sp[i];
    }
    und_context_ptr[15 + 1] = cur_sp[13]; //PC

    __asm volatile("push {r0-r6}");
     __asm volatile("ldr r0,=und_context_ptr");
     __asm volatile("ldr r0,[r0]");
     __asm volatile("mrs r1,spsr");
     __asm volatile("str r1,[r0]"); 

     __asm volatile("mrs r2,cpsr");
     __asm volatile("bic r3,r2,#0x1f");
     __asm volatile("orr r3,r3,#0x1f");
     __asm volatile("msr cpsr,r3");
     __asm volatile("mov r5,r13");
     __asm volatile("mov r6,r14");
     __asm volatile("msr cpsr,r2");
     __asm volatile("str r5,[r0, #56]"); //R13 (1+13) * 4
     __asm volatile("str r6,[r0, #60]"); //R14 (1+14) * 4
    __asm volatile("pop {r0-r6}");


    for(int i = 0 ; i < 17; i++)
    {
        printf("sys context:%d: %08lX\r\n",i, und_context_ptr[i]);
    }
        while (1);
    
      __asm volatile("add sp,sp,#56"); // 14*4

      __asm volatile("mov r6,sp");
      __asm volatile("mrs r1,cpsr_all");
      __asm volatile("bic r1,r1,#0x1f");
      __asm volatile("orr r1,r1,#0x1f");
      __asm volatile("msr cpsr_all,r1");
      //  __asm volatile("mov r7,sp"); 
      //  __asm volatile("ldr r8,=svc_stack");
      //  __asm volatile("ldr sp,[r8]");
      __asm volatile("mov sp,r6");
      //__asm volatile("sub sp,sp,#0x200");
      __asm volatile("mov lr,r7");

 
        __asm volatile("ldr r5,=und_context_ptr");
        __asm volatile("ldr r4,[r5]");
        __asm volatile("add r4,r4,#68"); //17*4
        __asm volatile("str r4,[r5]");
        
        __asm volatile("mov r1,#4");
        __asm volatile("ldr r2,=sys_dab_base");
        __asm volatile("ldr r2,[r2]");
        __asm volatile("bx r2"); 

    while (1)
        ;
}


#define LL_SWI_BASE (0xEE00)
#define LL_SWI_SET_IRQ_VECTOR         (LL_SWI_BASE + 8)
#define LL_SWI_SET_SVC_VECTOR         (LL_SWI_BASE + 13)
#define LL_SWI_SET_MEM_TRIM           (LL_SWI_BASE + 100)
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



uint32_t pab_stack = 0;
uint32_t dab_stack = 0;
uint32_t svc_stack = 0;

extern uint32_t side_load_pending_off ;
extern uint32_t side_load_pending_paddr ;

uint32_t svc_lr_ptr = 0;
uint32_t svc_cur_sp_ptr = 0;
uint32_t svc_lr[16];
uint32_t svc_cur_sp[16];

uint32_t swi_code;
extern lfs_t lfs;
extern const struct lfs_config cfg;

typedef struct fs_obj_t
{
    lfs_file_t file;
    struct lfs_file_config fcfg;
    uint32_t buffer[FS_CACHE_SZ / sizeof(uint32_t)];
}fs_obj_t;

typedef struct fs_dir_obj_t
{
    lfs_dir_t dir;
    struct lfs_info info;
}fs_dir_obj_t;



int trasnlateLfsErrCode(int err)
{
    switch (err)
    {
    case LFS_ERR_OK:
        return 0;
    case LFS_ERR_IO:
    case LFS_ERR_CORRUPT:
        return -EIO;
    case LFS_ERR_NOENT:
        return -ENOENT;
    case LFS_ERR_NOATTR:
    case LFS_ERR_EXIST:
        return -EEXIST;
    case LFS_ERR_NOTDIR:
        return -ENOTDIR;
    case LFS_ERR_ISDIR:
        return -EISDIR;
    case LFS_ERR_NOTEMPTY:
        return -ENOTEMPTY;
    case LFS_ERR_BADF:
        return -EBADF;
    case LFS_ERR_NOMEM:
        return -ENOMEM;
    case LFS_ERR_NOSPC:
    case LFS_ERR_NAMETOOLONG:
        return -ENOSPC;
    case LFS_ERR_INVAL:
        return -EINVAL;
    case LFS_ERR_FBIG:
        return -EFBIG;
    default:
        return err;
    }
}
void mem_chk_err(uint32_t code, uint32_t r0, uint32_t r1)
{
    printf("MEM CHK ERR:%08lX, %08lX , %08lX\r\n",code,r0 , r1);
}

uint32_t stack_jump_sys_svc = 0;
uint32_t after_svc_sp = 0;
uint32_t in_sys_svc = 0;
void __attribute__((target("arm")))  do_svc(uint32_t *cur_sp)
{
    uint32_t *swi_addr = ((uint32_t *)(cur_sp[13] - 4));

    //if(!mm_vaddr_map_pa((uint32_t)swi_addr))
    //{
    //    printf("load swi called page\r\n");
    //    do_dab(0xF5, (uint32_t)swi_addr, (uint32_t)swi_addr, SVC_MODE);
    //}
    
    
    swi_code = *swi_addr & 0xFFFFFF;
    
     //printf("swi cursp:%08lX\r\n",(uint32_t)cur_sp);
    //svc_lr_ptr--;
    //svc_cur_sp_ptr--;
    //printf("mode before swi:%02x\r\n", get_saved_mode());
    //printf("sp:%p\r\n", cur_sp);

    switch (swi_code) {
    case LL_SWI_FS_REMOVE:
        if(!is_addr_vaild(cur_sp[0], 0)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = trasnlateLfsErrCode(lfs_remove(&lfs, (const char *)cur_sp[0]));
        break;
    case LL_SWI_FS_RENAME:
        if(!is_addr_vaild(cur_sp[0], 0)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = trasnlateLfsErrCode(lfs_rename(&lfs, (const char *)cur_sp[0], (const char *)cur_sp[1]));
        break;
    case LL_SWI_FS_OPEN:
        {
            if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
            int ret;
            fs_obj_t *fobj = (fs_obj_t *)cur_sp[0];
            lfs_file_t *file = &fobj->file;
            struct lfs_file_config *fcfg = &fobj->fcfg;
            fcfg->attr_count = 0;
            fcfg->buffer = (void *)&fobj->buffer[0];
            int flags = cur_sp[2];
            int lfs_flags = 0;

            if ((flags & O_ACCMODE) == O_RDONLY) {
                lfs_flags = LFS_O_RDONLY;
            } else if ((flags & O_ACCMODE) == O_WRONLY) {
                lfs_flags = LFS_O_WRONLY;
            } else if ((flags & O_ACCMODE) == O_RDWR) {
                lfs_flags = LFS_O_RDWR;
            }

            if (flags & O_CREAT) {
                lfs_flags |= LFS_O_CREAT;
            }

            if (flags & O_EXCL) {
                lfs_flags |= LFS_O_EXCL;
            }

            if (flags & O_TRUNC) {
                lfs_flags |= LFS_O_TRUNC;
            }

            if (flags & O_APPEND) {
                lfs_flags |= LFS_O_APPEND;
            }

            ret = lfs_file_opencfg(&lfs, file, (const char *)cur_sp[1], lfs_flags, fcfg);
            cur_sp[0] = trasnlateLfsErrCode(ret);
        }
        break;
    case LL_SWI_FS_CLOSE:
        {
            if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
            cur_sp[0] = trasnlateLfsErrCode(lfs_file_close(&lfs, &((fs_obj_t *)cur_sp[0])->file));
        }
        break;
    case LL_SWI_FS_SYNC:
        if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = trasnlateLfsErrCode(lfs_file_sync(&lfs, &((fs_obj_t *)cur_sp[0])->file));
        break;
    case LL_SWI_FS_SIZE:
        if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = trasnlateLfsErrCode(lfs_file_size(&lfs, &((fs_obj_t *)cur_sp[0])->file));
        break;
    case LL_SWI_FS_READ:
        if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        if(!is_addr_vaild(cur_sp[1], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = trasnlateLfsErrCode(lfs_file_read(&lfs, &((fs_obj_t *)cur_sp[0])->file, (void *)cur_sp[1], cur_sp[2]));
        break;
    case LL_SWI_FS_WRITE:
        if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        if(!is_addr_vaild(cur_sp[1], 0)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = trasnlateLfsErrCode(lfs_file_write(&lfs, &((fs_obj_t *)cur_sp[0])->file, (void *)cur_sp[1], cur_sp[2]));
        break;
    case LL_SWI_FS_SEEK:
        if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = trasnlateLfsErrCode(lfs_file_seek(&lfs, &((fs_obj_t *)cur_sp[0])->file, cur_sp[1], cur_sp[2]));
        break;
    case LL_SWI_FS_REWIND:
        if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = trasnlateLfsErrCode(lfs_file_rewind(&lfs, &((fs_obj_t *)cur_sp[0])->file));
        break;
    case LL_SWI_FS_TRUNCATE:
        if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = trasnlateLfsErrCode(lfs_file_truncate(&lfs, &((fs_obj_t *)cur_sp[0])->file, cur_sp[1]));
        break;
    case LL_SWI_FS_TELL:
        if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = lfs_file_tell(&lfs, &((fs_obj_t *)cur_sp[0])->file);
        break;
    case LL_SWI_FS_DIR_MKDIR: 
        if(!is_addr_vaild(cur_sp[0], 0)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;} 
        cur_sp[0] = trasnlateLfsErrCode(lfs_mkdir(&lfs, (const char *)cur_sp[0])); 
        break;
    case LL_SWI_FS_DIR_OPEN:
        {
            if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
            if(!is_addr_vaild(cur_sp[1], 0)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
            fs_dir_obj_t *fs_dirobj = (fs_dir_obj_t *)cur_sp[0];
            lfs_dir_t *dirobj = &fs_dirobj->dir;
            cur_sp[0] = trasnlateLfsErrCode(lfs_dir_open(&lfs, dirobj, (const char *)cur_sp[1]));
        }
        break;
    case LL_SWI_FS_DIR_CLOSE:
        if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = trasnlateLfsErrCode(lfs_dir_close(&lfs, &((fs_dir_obj_t *)cur_sp[0])->dir   ));
        break;
    case LL_SWI_FS_DIR_SEEK:
        if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = trasnlateLfsErrCode(lfs_dir_seek(&lfs, &((fs_dir_obj_t *)cur_sp[0])->dir, cur_sp[1]));
        break;
    case LL_SWI_FS_DIR_TELL:
        if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = lfs_dir_tell(&lfs, &((fs_dir_obj_t *)cur_sp[0])->dir);
        break;
    case LL_SWI_FS_DIR_REWIND:
        if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
        cur_sp[0] = trasnlateLfsErrCode(lfs_dir_rewind(&lfs, &((fs_dir_obj_t *)cur_sp[0])->dir ));
        break;
    case LL_SWI_FS_DIR_READ:
        {
            if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
            fs_dir_obj_t *fs_dirobj = (fs_dir_obj_t *)cur_sp[0];
            struct lfs_info *dir_info = &fs_dirobj->info;
            cur_sp[0] = trasnlateLfsErrCode(lfs_dir_read(&lfs, &fs_dirobj->dir, dir_info));
        }
        break;
    case LL_SWI_FS_DIR_GET_CUR_TYPE:
        {
            if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
            fs_dir_obj_t *fs_dirobj = (fs_dir_obj_t *)cur_sp[0];
            struct lfs_info *dir_info = &fs_dirobj->info;
            cur_sp[0] = dir_info->type;
        }
        break;

    case LL_SWI_FS_DIR_GET_CUR_NAME:
        {
            if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
            fs_dir_obj_t *fs_dirobj = (fs_dir_obj_t *)cur_sp[0];
            struct lfs_info *dir_info = &fs_dirobj->info;
            cur_sp[0] = (uint32_t)dir_info->name;
        }
        break;

    case LL_SWI_FS_DIR_GET_CUR_SIZE:
        {
            if(!is_addr_vaild(cur_sp[0], 1)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
            fs_dir_obj_t *fs_dirobj = (fs_dir_obj_t *)cur_sp[0];
            struct lfs_info *dir_info = &fs_dirobj->info;
            cur_sp[0] = dir_info->size;
        }
        break;
    case LL_SWI_FS_GET_FOBJ_SZ:
        cur_sp[0] = sizeof(fs_obj_t);
        break;
    case LL_SWI_FS_GET_DIROBJ_SZ:
        cur_sp[0] = sizeof(fs_dir_obj_t);
        break;

    case LL_SWI_SET_IRQ_VECTOR:
        sys_irq_base = (void *)cur_sp[0];
        printf("SET SYS IRQ BASE:%8p\r\n", sys_irq_base);
        break;
    case LL_SWI_SET_SVC_VECTOR:
        sys_svc_base = (void *)cur_sp[0];
        printf("SET SYS SWI BASE:%p\r\n", sys_svc_base);
        break;
    case LL_SWI_SET_DAB_VECTOR:
        sys_dab_base = (void *)cur_sp[0];
        printf("SET SYS ABT BASE:%8p\r\n", sys_dab_base);
        break;
    case LL_SWI_SET_PAB_VECTOR:
        sys_pab_base = (void *)cur_sp[0];
        printf("SET SYS PAB BASE:%p\r\n", sys_pab_base);
        break;
    case LL_SWI_SET_DAB_STACK:
        dab_stack = cur_sp[0];
        printf("SET SYS PAB STACK:%08lX\r\n", dab_stack);
        break;
    case LL_SWI_SET_PAB_STACK:
        pab_stack = cur_sp[0];
        printf("SET SYS PAB STACK:%08lX\r\n", pab_stack);
        break;
    case LL_SWI_SET_SVC_STACK:
        svc_stack = cur_sp[0];
        printf("SET SYS SVC STACK:%08lX\r\n", svc_stack);
        break;
    case LL_SWI_EXIT_SYS_SVC:
        in_sys_svc--;
        break;
    case LL_SWI_SET_MEM_TRIM:
        cur_sp[0] = mm_trim_page(cur_sp[0]);
        break;
    case LL_SWI_MEM_IS_VAILD:
        cur_sp[0] = is_addr_vaild(cur_sp[0], 0);
        break;
    case LL_SWI_SET_APP_MEM_WARP:
        mm_set_app_mem_warp(cur_sp[0], cur_sp[1]);
        break;
    case LL_SWI_GET_PENDING_PGADDR:
        cur_sp[0] = side_load_pending_paddr;
        break;
    case LL_SWI_GET_PENDING_PGFOFF:
        cur_sp[0] = side_load_pending_off;
        break;
    case LL_SWI_GET_VADDR_PA:
        cur_sp[0] = mm_vaddr_map_pa(cur_sp[0]);
        break;
    case LL_SWI_GET_VADDR_LOCK:
        cur_sp[0] = mm_lock_vaddr(cur_sp[0], cur_sp[1]);
        break;
    case LL_SWI_MMAP:
        {
            if(!is_addr_vaild(cur_sp[0], 0)){mem_chk_err(swi_code, cur_sp[0], cur_sp[1]); cur_sp[0] = -EINVAL; break;}
            mmap_info *info = (mmap_info *)cur_sp[0];
            printf("create mmap:%08lX, %s, %08lX, %ld, %d, %d\r\n", info->map_to, info->path, info->offset, info->size, info->writable, info->writeback);
            
            int ret = mmap(info->map_to, info->path, info->offset, info->size, info->writable, info->writeback);
            cur_sp[0] = ret;
        }
        break;
    case LL_SWI_MUNMAP:
        munmap(cur_sp[0]); 
        break;
    default:
        if (sys_svc_base) 
        {
            //svc_lr_ptr++;
            //svc_cur_sp_ptr++;

            //in_sys_svc++;

            //if(!mm_vaddr_map_pa((uint32_t)sys_svc_base))
            //{
            //    printf("load svc syssvc page\r\n");
            //    do_dab(0xF5, (uint32_t)sys_svc_base, (uint32_t)sys_svc_base, SVC_MODE);
            //}
            //printf("svc sp:%08lx\r\n", svc_stack);
            //__asm volatile("ldr r2,=svc_cur_sp");
            //__asm volatile("ldr sp,[r2]");
            //__asm volatile("ldr r2,=svc_lr");
            //__asm volatile("ldr lr,[r2]");
            //printf("svc_lr_ptr:%ld\r\n",svc_lr_ptr);
            //printf("val:%08lX\r\n",svc_lr[svc_lr_ptr] );
            //printf("svc_cur_sp_ptr:%ld\n",svc_cur_sp_ptr);
            //printf("val:%08lX\r\n",svc_cur_sp[svc_cur_sp_ptr] );
             

            for(int i = 0; i <= 12; i++)
            {
                svc_context_ptr[i+1] = cur_sp[i];
            }
            svc_context_ptr[15 + 1] = cur_sp[13]; //PC


            //__asm volatile("ldr r1,=svc_lr");
            //__asm volatile("ldr r2,=svc_lr_ptr");
            //__asm volatile("ldr r3,[r2]");
            //__asm volatile("sub r3,r3,#1");
            //__asm volatile("str r3,[r2]");
            //__asm volatile("lsl r3,r3,#2");
            //__asm volatile("add r1,r1,r3");
            //__asm volatile("ldr lr,[r1]");


            //__asm volatile("ldr r1,=svc_cur_sp");
            //__asm volatile("ldr r2,=svc_cur_sp_ptr");
            //__asm volatile("ldr r3,[r2]");
            //__asm volatile("sub r3,r3,#1");
            //__asm volatile("str r3,[r2]");
            //__asm volatile("lsl r3,r3,#2");
            //__asm volatile("add r1,r1,r3");
            //__asm volatile("ldr sp,[r1]");


                __asm volatile("ldr r0,=svc_context_ptr");
                __asm volatile("ldr r0,[r0]");
                __asm volatile("mrs r1,spsr");
                __asm volatile("str r1,[r0]"); 

               __asm volatile("mrs r2,cpsr");
               __asm volatile("bic r3,r2,#0x1f");
               //__asm volatile("bic r1,r1,#0x1f");
               //__asm volatile("orr r3,r3,r1");
               __asm volatile("orr r3,r3,#0x1f");
               __asm volatile("msr cpsr,r3");
               __asm volatile("mov r5,r13");
               __asm volatile("mov r6,r14");
               __asm volatile("msr cpsr,r2");
               __asm volatile("str r5,[r0, #56]"); //R13 (1+13) * 4
               __asm volatile("str r6,[r0, #60]"); //R14 (1+14) * 4


                /*__asm volatile("\
                  ldr r1,=in_sys_svc          \n\
                  ldr r2,=stack_jump_sys_svc  \n\
                  ldr r3,=after_svc_sp        \n\
                  ldr r3,[r3]                 \n\
                  ldr r1,[r1]                 \n\
                  cmp r1,#1                   \n\
                  strls r3,[r2]               \n\
                  strhi r5,[r2]               \n\
                  ");
                */

               //if(in_sys_svc > 1)
               //{
               // stack_jump_sys_svc = svc_context_ptr[13];
               //}else{
               // stack_jump_sys_svc = after_svc_sp;
               //}



            __asm volatile("ldr r1,=after_svc_sp");
            __asm volatile("ldr sp,[r1]");
            __asm volatile("add sp,sp,#56"); // 14*4

            __asm volatile("mov r6,sp");
            __asm volatile("mrs r1,cpsr_all");
            __asm volatile("bic r1,r1,#0x1f");
            __asm volatile("orr r1,r1,#0x1f");
            __asm volatile("msr cpsr_all,r1");
            //  __asm volatile("mov r7,sp"); 
            //  __asm volatile("ldr r8,=svc_stack");
            //  __asm volatile("ldr sp,[r8]");
            __asm volatile("mov sp,r6");
            //__asm volatile("sub sp,sp,#0x200");
        //__asm volatile("mov lr,r7");

 
        __asm volatile("ldr r5,=svc_context_ptr");
        __asm volatile("ldr r4,[r5]");
        __asm volatile("add r4,r4,#68"); //17*4
        __asm volatile("str r4,[r5]");

              //__asm volatile("mov r7,sp");
              //__asm volatile("mov r8,lr");
          //    __asm volatile("mrs r1,cpsr_all");
          //    __asm volatile("bic r1,r1,#0x1f");
          //    __asm volatile("orr r1,r1,#0x1f");
          //    __asm volatile("msr cpsr_all,r1");
              //__asm volatile("mov sp,r7");
              //__asm volatile("mov lr,r8");
             // __asm volatile("mov r7,sp");
        //     __asm volatile("ldr r8,=svc_stack");
        //     __asm volatile("ldr sp,[r8]");

            __asm volatile("ldr r0,=swi_code");
            __asm volatile("ldr r0,[r0]");

            __asm volatile("ldr r1,=sys_svc_base");
            __asm volatile("ldr r1,[r1]");
 
            __asm volatile("bx r1");  

            

        } else {
            printf("\n[NOT Handled SWI]at:%08lX,R0=%08lX,r1=%08lX,code=%08lx\n", cur_sp[13], cur_sp[0], cur_sp[1], swi_code);
            for (int i = 0; i < 15; i++) {
                printf("A,R%d, %p:%08lx\r\n", i, &cur_sp[i], cur_sp[i]);
            }
            while (1)
                ;
        }
        break;
    }
}


void volatile __attribute__((target("arm"))) __attribute__((naked)) arm_vector_swi() {
    __asm volatile("stmdb sp!, {r0-r12, lr}"); 
    __asm volatile("ldr r1,=after_svc_sp");
    __asm volatile("str sp,[r1]");



    //__asm volatile("ldr r1,=svc_lr");
    //__asm volatile("ldr r2,=svc_lr_ptr");
    //__asm volatile("ldr r3,[r2]");
    //__asm volatile("lsl r4,r3,#2");
    //__asm volatile("add r1,r1,r4");
    //__asm volatile("str lr,[r1]");
    //__asm volatile("add r3,r3,#1");
    //__asm volatile("str r3,[r2]");
//
//
    //__asm volatile("ldr r1,=svc_cur_sp");
    //__asm volatile("ldr r2,=svc_cur_sp_ptr");
    //__asm volatile("ldr r3,[r2]");
    //__asm volatile("lsl r4,r3,#2");
    //__asm volatile("add r1,r1,r4");
    //__asm volatile("str sp,[r1]");
    //__asm volatile("add r3,r3,#1");
    //__asm volatile("str r3,[r2]");

    __asm volatile("mov r0, sp");
    __asm volatile("bl do_svc");

    __asm volatile("ldmia sp!, {r0-r12, pc}^");

}

void volatile __attribute__((target("arm"))) __attribute__((naked)) arm_vector_irq() {
    
    __asm volatile("sub lr,lr,#4");
    __asm volatile("stmdb sp!, {r0-r12, lr}");



   //__asm volatile("mrs r3,cpsr_all");
   //__asm volatile("bic r2,r3,#0x1f");
   //__asm volatile("orr r2,r2,#0x1f");
   //__asm volatile("msr cpsr_all,r2");

    __asm volatile("ldr r1,=sys_irq_base");
    __asm volatile("ldr r1,[r1]");

   // __asm volatile("ldr r0,[r1]");
   // __asm volatile("msr cpsr_all,r3");


    __asm volatile("bx r1");
    //while(1);
}


uint32_t abt_cur_sp[16];
uint32_t abt_cur_sp_ptr = 0;

//uint32_t dab_cur_sp[16];
//uint32_t dab_cur_sp_ptr = 0;

uint32_t dab_lr[16];
uint32_t dab_lr_ptr = 0;

uint32_t pab_lr[16];
uint32_t pab_lr_ptr = 0;

void volatile __attribute__((target("arm"))) pre_do_pab(uint32_t *cur_sp, uint32_t faddr) {

    int ret = 0;
    //pab_cur_sp = (uint32_t)cur_sp;
   // printf("pab cursp:%08lX\r\n",(uint32_t)cur_sp);
        //
    ret = do_dab(0xF5, cur_sp[13], cur_sp[13], get_spsr() & 0x1F);
    pab_lr_ptr--;
    abt_cur_sp_ptr--;
    if (ret) {
        if(ret == -1)
        {
            for (int i = 0; i < 14; i++) {
                printf(" C: %d, %p:%08lx\r\n", i, &cur_sp[i], cur_sp[i]);
            }
            printf("[!!!PAB]FSR:%02X, at:%08lX, M:%02lX \r\n", 0xF5, cur_sp[13], get_spsr() & 0x1F);
            //while(1);
        }
        //printf("[ PAB]FSR:%02X, at:%08lX \r\n", 0xF5, cur_sp[13]);
        
        //  pab_lr_ptr++;
        //  abt_cur_sp_ptr++;

          //  __asm volatile("ldr r1,=pab_lr");
          //  __asm volatile("ldr r2,=pab_lr_ptr");
          //  __asm volatile("ldr r3,[r2]");
          //  __asm volatile("sub r3,r3,#1");
          //  __asm volatile("str r3,[r2]");
          //  __asm volatile("lsl r3,r3,#2");
          //  __asm volatile("add r1,r1,r3");
          //  __asm volatile("ldr lr,[r1]");
//
//
          // __asm volatile("ldr r1,=abt_cur_sp");
          // __asm volatile("ldr r2,=abt_cur_sp_ptr");
          // __asm volatile("ldr r3,[r2]");
          // __asm volatile("sub r3,r3,#1");
          // __asm volatile("str r3,[r2]");
          // __asm volatile("lsl r3,r3,#2");
          // __asm volatile("add r1,r1,r3");
          // __asm volatile("ldr sp,[r1]");
//
        //
          // __asm volatile("mrs r1,cpsr_all");
          // __asm volatile("bic r1,r1,#0x1f");
          // __asm volatile("orr r1,r1,#0x1b");
          // __asm volatile("msr cpsr_all,r1");
          // __asm volatile("ldr r8,=pab_stack");
          // __asm volatile("ldr sp,[r8]");
//
          // __asm volatile("ldr r2,=sys_dab_base");
          // __asm volatile("ldr r2,[r2]");
          // __asm volatile("mov r1,lr");
          // __asm volatile("mov r0,#1");
          // __asm volatile("bx r2");

         //printf("pab cursp:%08lX\r\n",(uint32_t)cur_sp);
       // printf("[PAB]FSR:%02X, at:%08lX \r\n", 5, cur_sp[13]);
       // printf("pab fault mode:%02lX\r\n", get_spsr() & 0x1F);
           
       // printf("b1 pab_context_ptr:%08lX\r\n", (uint32_t)pab_context_ptr);
     
        for(int i = 0; i <= 12; i++)
        {
            pab_context_ptr[i+1] = cur_sp[i];
        }
        pab_context_ptr[15 + 1] = cur_sp[13]; //PC
        
        pab_lr_ptr++;
        abt_cur_sp_ptr++;

        if(ret != -1)
        {
            __asm volatile("mov r9,#1");
        }else{
            __asm volatile("mov r9,#3");
        }

          __asm volatile("ldr r1,=pab_lr");
          __asm volatile("ldr r2,=pab_lr_ptr");
          __asm volatile("ldr r3,[r2]");
          __asm volatile("sub r3,r3,#1");
          __asm volatile("str r3,[r2]");
          __asm volatile("lsl r3,r3,#2");
          __asm volatile("add r1,r1,r3");
          __asm volatile("ldr lr,[r1]");


         __asm volatile("ldr r1,=abt_cur_sp");
         __asm volatile("ldr r2,=abt_cur_sp_ptr");
         __asm volatile("ldr r3,[r2]");
         __asm volatile("sub r3,r3,#1");
         __asm volatile("str r3,[r2]");
         __asm volatile("lsl r3,r3,#2");
         __asm volatile("add r1,r1,r3");
         __asm volatile("ldr sp,[r1]");

        __asm volatile("ldr r0,=pab_context_ptr");
        __asm volatile("ldr r0,[r0]");
        __asm volatile("mrs r1,spsr");
        __asm volatile("str r1,[r0]");
        __asm volatile("movs r1,r13");

        //__asm volatile("add r0,r0,#56");
        //__asm volatile("stmia r0,{r13}^");
        ////__asm volatile("add r0,r0,#4");
        //__asm volatile("stmia r0,{r14}^");

        
        //__asm volatile("movs r1,r13");
         __asm volatile("mrs r2,cpsr");
         __asm volatile("bic r3,r2,#0x1f");
         __asm volatile("orr r3,r3,#0x1f");
         __asm volatile("msr cpsr,r3");
         __asm volatile("mov r5,r13");
         __asm volatile("mov r6,r14");
         __asm volatile("msr cpsr,r2");
         __asm volatile("str r5,[r0, #56]"); //R13 (1+13) * 4
         __asm volatile("str r6,[r0, #60]"); //R14 (1+14) * 4
         
        

        //__asm volatile("ldr r4,=pab_cur_sp");
        //__asm volatile("ldr sp,[r4]");
        //__asm volatile("ldr r3,=pab_lr");
        //__asm volatile("ldr lr,[r3]");



     
        //__asm volatile("mov r7,lr");
          __asm volatile("add sp,sp,#56"); // 14*4
              __asm volatile("mov r6,sp");
               __asm volatile("mrs r1,cpsr_all");
               __asm volatile("bic r1,r1,#0x1f");
               __asm volatile("orr r1,r1,#0x1f");
               __asm volatile("msr cpsr_all,r1");
              //__asm volatile("mov r7,sp");
              //__asm volatile("ldr r8,=pab_stack");
              //__asm volatile("ldr sp,[r8]");
              __asm volatile("mov sp,r6");
        //__asm volatile("mov lr,r7");


        __asm volatile("ldr r1,=pab_lr");
        __asm volatile("ldr r1,[r1]");
        __asm volatile("mov r0,r9");
        __asm volatile("ldr r5,=pab_context_ptr");
        __asm volatile("ldr r4,[r5]");
        __asm volatile("add r4,r4,#68"); //17*4
        __asm volatile("str r4,[r5]");

        __asm volatile("ldr r2,=sys_pab_base");
        __asm volatile("ldr r2,[r2]");
        __asm volatile("bx r2"); 

/*
        __asm volatile("ldr r4,=pab_cur_sp");
        __asm volatile("ldr sp,[r4]");
        __asm volatile("ldr r3,=pab_lr");
        __asm volatile("ldr lr,[r3]");


        __asm volatile("mrs r6,spsr");
        __asm volatile("mov r9,sp");
        __asm volatile("mov r7,lr");
        __asm volatile("mov r8,#0x1b");
        __asm volatile("mrs r1,cpsr_all");
        __asm volatile("bic r1,r1,#0x1f");
        __asm volatile("orr r1,r1,r8");
        __asm volatile("msr cpsr_all,r1");
        __asm volatile("mov r0,r0");
        __asm volatile("mov r0,r0");
        __asm volatile("mov sp,r9");
        __asm volatile("mov lr,r7");
        __asm volatile("msr spsr,r6");

        __asm volatile("ldr r2,=sys_dab_base");
        __asm volatile("ldr r2,[r2]");
        __asm volatile("ldr r1,[r3]");
        __asm volatile("mov r0,#1");
        __asm volatile("bx r2"); */
        while (1)
            ;
    }

}


void volatile __attribute__((target("arm"))) pre_do_dab(uint32_t *cur_sp, uint32_t faultAddress, uint32_t FSR)
{
    int ret = 0;
    //dab_cur_sp = (uint32_t)cur_sp;
   // printf("dab cursp:%08lX\r\n",(uint32_t)cur_sp);
     //   
    ret = do_dab(FSR, faultAddress, cur_sp[13], get_spsr() & 0x1F);
    dab_lr_ptr--;
    abt_cur_sp_ptr--;

    if (ret) {
        if(ret == -1)
        {        
            for (int i = 0; i < 14; i++) {
                printf(" C: %d, %p:%08lx\r\n", i, &cur_sp[i], cur_sp[i]);
            }
            printf("[!!!dab]FSR:%02lX, access:%08lx, at:%08lX,  M:%02lX \r\n", FSR, faultAddress, cur_sp[13], get_spsr() & 0x1F);
            //while(1);
        }    
        
        //printf("sys_dab_base:%08lX\r\n", ((uint32_t *)sys_dab_base)[0]);
        
        //printf("[  dab]FSR:%02lX, access:%08lx, at:%08lX\r\n", FSR, faultAddress, cur_sp[13]);
        
            //dab_lr_ptr++;
            //abt_cur_sp_ptr++;
            // __asm volatile("ldr r1,=dab_lr");
            // __asm volatile("ldr r2,=dab_lr_ptr");
            // __asm volatile("ldr r3,[r2]");
            // __asm volatile("sub r3,r3,#1");
            // __asm volatile("str r3,[r2]");
            // __asm volatile("lsl r3,r3,#2");
            // __asm volatile("add r1,r1,r3");
            // __asm volatile("ldr lr,[r1]");
            // 
            // __asm volatile("ldr r1,=abt_cur_sp");
            // __asm volatile("ldr r2,=abt_cur_sp_ptr");
            // __asm volatile("ldr r3,[r2]");
            // __asm volatile("sub r3,r3,#1");
            // __asm volatile("str r3,[r2]");
            // __asm volatile("lsl r3,r3,#2");
            // __asm volatile("add r1,r1,r3");
            // __asm volatile("ldr sp,[r1]");
    //
    //
            // 
            //   __asm volatile("mrs r1,cpsr_all");
            //   __asm volatile("bic r1,r1,#0x1f");
            //   __asm volatile("orr r1,r1,#0x13");
            //   __asm volatile("msr cpsr_all,r1");
            //   __asm volatile("mov r7,sp");
            //   __asm volatile("ldr r8,=dab_stack");
            //   __asm volatile("ldr sp,[r8]");
           //
            //__asm volatile("ldr r2,=sys_dab_base");
            //__asm volatile("ldr r2,[r2]");
            //__asm volatile("mrc p15, 0, r1, c6, c0, 0"); //faultAddress
            //__asm volatile("mov r0,#0");
            //__asm volatile("bx r2"); 

      //  printf("[dab]FSR:%02lX, access:%08lx, at:%08lX\r\n", FSR, faultAddress, cur_sp[13]);
      //  printf("dab fault mode:%02lX\r\n", get_spsr() & 0x1F);
          //printf("dab cursp:%08lX\r\n",(uint32_t)cur_sp);
        
      //  printf("b1 dab_context_ptr:%08lX\r\n", (uint32_t)dab_context_ptr);
           for(int i = 0; i <=  12; i++)
           {
               dab_context_ptr[i+1] = cur_sp[i];
           }
           dab_context_ptr[15 + 1] = cur_sp[13]; //PC
    
           dab_lr_ptr++;
           abt_cur_sp_ptr++;
    
           if(ret != -1)
           {
               __asm volatile("mov r9,#0");
           }else{
               __asm volatile("mov r9,#3");
           }
    
               __asm volatile("ldr r1,=dab_lr");
               __asm volatile("ldr r2,=dab_lr_ptr");
               __asm volatile("ldr r3,[r2]");
               __asm volatile("sub r3,r3,#1");
               __asm volatile("str r3,[r2]");
               __asm volatile("lsl r3,r3,#2");
               __asm volatile("add r1,r1,r3");
               __asm volatile("ldr lr,[r1]");
    
    
               __asm volatile("ldr r1,=abt_cur_sp");
               __asm volatile("ldr r2,=abt_cur_sp_ptr");
               __asm volatile("ldr r3,[r2]");
               __asm volatile("sub r3,r3,#1");
               __asm volatile("str r3,[r2]");
               __asm volatile("lsl r3,r3,#2");
               __asm volatile("add r1,r1,r3");
               __asm volatile("ldr sp,[r1]");
    
               __asm volatile("ldr r0,=dab_context_ptr");
               __asm volatile("ldr r0,[r0]");
               __asm volatile("mrs r1,spsr");
               __asm volatile("str r1,[r0]"); 
    
               __asm volatile("mrs r2,cpsr");
               __asm volatile("bic r3,r2,#0x1f");
               __asm volatile("orr r3,r3,#0x1f");
               __asm volatile("msr cpsr,r3");
               __asm volatile("mov r5,r13");
               __asm volatile("mov r6,r14");
               __asm volatile("msr cpsr,r2");
               __asm volatile("str r5,[r0, #56]"); //R13 (1+13) * 4
               __asm volatile("str r6,[r0, #60]"); //R14 (1+14) * 4

             //__asm volatile("mrs r2,cpsr");
             //__asm volatile("bic r3,r2,#0x1f");
             //__asm volatile("orr r3,r3,#0x1f");
             //__asm volatile("msr cpsr,r3");
             //__asm volatile("mov r5,r13");
             //__asm volatile("mov r6,r14");
             //__asm volatile("msr cpsr,r2");
             //__asm volatile("str r5,[r0, #56]"); //R13 (1+13) * 4
             //__asm volatile("str r6,[r0, #60]"); //R14 (1+14) * 4
    
           //__asm volatile("movs r1,r13");
           //__asm volatile("str r1,[r0, #56]"); //R13 (1+13) * 4
           //__asm volatile("movs r1,r14");
           //__asm volatile("str r1,[r0, #60]"); //R14 (1+14) * 4
           
    
           //__asm volatile("ldr r4,=dab_cur_sp");
           //__asm volatile("ldr sp,[r4]");
           //__asm volatile("ldr r3,=dab_lr");
           //__asm volatile("ldr lr,[r3]");
    
    
    
           //__asm volatile("mov r7,lr"); 
            __asm volatile("add sp,sp,#56"); // 14*4
               __asm volatile("mov r6,sp");
                   __asm volatile("mrs r1,cpsr_all");
                   __asm volatile("bic r1,r1,#0x1f");
                   __asm volatile("orr r1,r1,#0x1f");
                   __asm volatile("msr cpsr_all,r1");
                 __asm volatile("mov sp,r6");
            //     __asm volatile("ldr r8,=dab_stack");
            //     __asm volatile("ldr sp,[r8]");
           //__asm volatile("mov lr,r7");
    
           __asm volatile("ldr r2,=sys_dab_base");
           __asm volatile("ldr r2,[r2]");
           __asm volatile("mov r0,r9");
           __asm volatile("mrc p15, 0, r1, c6, c0, 0"); //faultAddress
           __asm volatile("ldr r5,=dab_context_ptr");
           __asm volatile("ldr r4,[r5]");
           __asm volatile("add r4,r4,#68"); //17 * 4
           __asm volatile("str r4,[r5]");
    
           __asm volatile("bx r2"); 
    
        /*
        __asm volatile("ldr r4,=dab_cur_sp");
        __asm volatile("ldr r4,[r4]");
        __asm volatile("ldr r3,=dab_lr");
        __asm volatile("ldr lr,[r3]");

        
        __asm volatile("mrs r6,spsr");
        __asm volatile("mov r9,sp");
        __asm volatile("mov r7,lr");
        __asm volatile("mov r8,#0x1b");
        __asm volatile("mrs r1,cpsr_all");
        __asm volatile("bic r1,r1,#0x1f");
        __asm volatile("orr r1,r1,r8");
        __asm volatile("msr cpsr_all,r1");
        
        __asm volatile("mov r0,r0");
        __asm volatile("mov r0,r0");
        __asm volatile("mov sp,r9");
        __asm volatile("mov lr,r7");
        __asm volatile("msr spsr,r6");

        __asm volatile("ldr r2,=sys_dab_base");
        __asm volatile("ldr r2,[r2]");
        __asm volatile("mrc p15, 0, r1, c6, c0, 0"); //faultAddress
        __asm volatile("mov r0,#0");
        __asm volatile("bx r2"); 


        while (1)
            ;*/
    }
}

void volatile __attribute__((target("arm"))) __attribute__((naked)) arm_vector_pab() {

    __asm volatile("sub  lr,lr,#4");
    __asm volatile("stmdb sp!, {r0-r12, lr}");
 

    __asm volatile("ldr r1,=pab_lr");
    __asm volatile("ldr r2,=pab_lr_ptr");
    __asm volatile("ldr r3,[r2]");
    __asm volatile("lsl r4,r3,#2");
    __asm volatile("add r1,r1,r4");
    __asm volatile("str lr,[r1]");
    __asm volatile("add r3,r3,#1");
    __asm volatile("str r3,[r2]");


    __asm volatile("ldr r1,=abt_cur_sp");
    __asm volatile("ldr r2,=abt_cur_sp_ptr");
    __asm volatile("ldr r3,[r2]");
    __asm volatile("lsl r4,r3,#2");
    __asm volatile("add r1,r1,r4");
    __asm volatile("str sp,[r1]");
    __asm volatile("add r3,r3,#1");
    __asm volatile("str r3,[r2]");
    //__asm volatile("ldr r1,=pab_lr");
    //__asm volatile("str lr,[r1]");

    __asm volatile("mov r0, sp");
    __asm volatile("mov r1, lr");
    __asm volatile("bl pre_do_pab");

    __asm volatile("ldmia sp!, {r0-r12, pc}^");
}

void volatile __attribute__((target("arm"))) __attribute__((naked)) arm_vector_dab() {

    __asm volatile("sub  lr,lr,#8");
    __asm volatile("stmdb sp!, {r0-r12, lr}"); 


    __asm volatile("ldr r1,=dab_lr");
    __asm volatile("ldr r2,=dab_lr_ptr");
    __asm volatile("ldr r3,[r2]");
    __asm volatile("lsl r4,r3,#2");
    __asm volatile("add r1,r1,r4");
    __asm volatile("str lr,[r1]");
    __asm volatile("add r3,r3,#1");
    __asm volatile("str r3,[r2]");


    __asm volatile("ldr r1,=abt_cur_sp");
    __asm volatile("ldr r2,=abt_cur_sp_ptr");
    __asm volatile("ldr r3,[r2]");
    __asm volatile("lsl r4,r3,#2");
    __asm volatile("add r1,r1,r4");
    __asm volatile("str sp,[r1]");
    __asm volatile("add r3,r3,#1");
    __asm volatile("str r3,[r2]"); 

    __asm volatile("mov r0,sp");
    __asm volatile("mrc p15, 0, r1, c6, c0, 0"); //faultAddress
    __asm volatile("mrc p15, 0, r2, c5, c0, 0"); //fsr
    __asm volatile("bl pre_do_dab");
 
    __asm volatile("ldmia sp!, {r0-r12, pc}^"); 
}


void volatile __attribute__((target("arm"))) __attribute__((naked)) arm_vector_fiq() {
    print_uart0("1fiq\r\n");
    while (1)
        ;
}
