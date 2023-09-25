#pragma once

#include <stdint.h>
#include <stdbool.h>


#define HCATTR   __attribute__((naked)) __attribute__((target("arm")))
#define DECDEF_LLAPI_SWI(ret, name, pars, SWINum)   \
    ret HCATTR name pars;


#define LL_SWI_BASE (0xEE00)
#define LLAPI_SWI_BASE                     (0xD700)

#define LLAPI_APP_DELAY_MS                 (LLAPI_SWI_BASE + 1)
#define LLAPI_APP_STDOUT_PUTC              (LLAPI_SWI_BASE + 2)
#define LLAPI_APP_GET_RAM_SIZE             (LLAPI_SWI_BASE + 3)
#define LLAPI_APP_GET_TICK_MS              (LLAPI_SWI_BASE + 4)
#define LLAPI_APP_GET_TICK_US              (LLAPI_SWI_BASE + 5)
#define LLAPI_APP_DISP_PUT_P               (LLAPI_SWI_BASE + 6)
#define LLAPI_APP_DISP_PUT_HLINE           (LLAPI_SWI_BASE + 7)
#define LLAPI_APP_DISP_GET_P               (LLAPI_SWI_BASE + 8)
#define LLAPI_APP_QUERY_KEY                (LLAPI_SWI_BASE + 9)
#define LLAPI_APP_RTC_GET_S                (LLAPI_SWI_BASE + 10)
#define LLAPI_APP_RTC_SET_S                (LLAPI_SWI_BASE + 11)
//#define LLAPI_APP_DISP_PUT_BUFFER           (LLAPI_SWI_BASE + 12)

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


#define FS_FILE_TYPE_REG   (1)
#define FS_FILE_TYPE_DIR   (2)

typedef void* fs_obj_t;
typedef void* fs_dir_obj_t;
/*
typedef struct Coords
{
    uint32_t x;
    uint32_t y;
}Coords;
*/
#ifdef __cplusplus
    extern "C" {
#endif


DECDEF_LLAPI_SWI(void,          llapi_delay_ms,              (uint32_t ms),                               LLAPI_APP_DELAY_MS)
DECDEF_LLAPI_SWI(void,          llapi_putc,                  (char c),                                    LLAPI_APP_STDOUT_PUTC)
DECDEF_LLAPI_SWI(uint32_t,      llapi_get_ram_size,          (void),                                      LLAPI_APP_GET_RAM_SIZE)
DECDEF_LLAPI_SWI(uint32_t,      llapi_get_tick_ms,           (void),                                      LLAPI_APP_GET_TICK_MS)
DECDEF_LLAPI_SWI(uint32_t,      llapi_get_tick_us,           (void),                                      LLAPI_APP_GET_TICK_US)
DECDEF_LLAPI_SWI(int,           llapi_query_key,             (void),                                      LLAPI_APP_QUERY_KEY)
DECDEF_LLAPI_SWI(int,           llapi_rtc_get_s,             (void),                                      LLAPI_APP_RTC_GET_S)
DECDEF_LLAPI_SWI(uint32_t,      llapi_rtc_set_s,             (uint32_t s),                                LLAPI_APP_RTC_SET_S)
    
DECDEF_LLAPI_SWI(void,          llapi_disp_put_point,        (uint32_t x, uint32_t y, int c),             LLAPI_APP_DISP_PUT_P)
DECDEF_LLAPI_SWI(int,           llapi_disp_get_point,        (uint32_t x, uint32_t y),                    LLAPI_APP_DISP_GET_P)
DECDEF_LLAPI_SWI(void,          llapi_disp_put_hline,        (uint32_t y, char *dat),                     LLAPI_APP_DISP_PUT_HLINE)
//DECDEF_LLAPI_SWI(void,          llapi_disp_put_buffer,       (uint8_t* buffer,uint32_t width,uint32_t height,Coords *coord),LLAPI_APP_DISP_PUT_BUFFER);

DECDEF_LLAPI_SWI(uint32_t,      llapi_fs_get_dirobj_sz,      (void)                                      ,LL_SWI_FS_GET_DIROBJ_SZ      );
DECDEF_LLAPI_SWI(uint32_t,      llapi_fs_get_fobj_sz,        (void)                                      ,LL_SWI_FS_GET_FOBJ_SZ        );
DECDEF_LLAPI_SWI(int,           ll_fs_size,                  (fs_obj_t fobj)                             ,LL_SWI_FS_SIZE               );
DECDEF_LLAPI_SWI(int,           llapi_fs_remove,             (const char *path)                          ,LL_SWI_FS_REMOVE             );
DECDEF_LLAPI_SWI(int,           llapi_fs_rename,             (const char *oldpath, const char *newpath)  ,LL_SWI_FS_RENAME             );
DECDEF_LLAPI_SWI(int,           llapi_fs_open,               (fs_obj_t fobj, const char *path, int flag) ,LL_SWI_FS_OPEN               );
DECDEF_LLAPI_SWI(int,           llapi_fs_close,              (fs_obj_t fobj)                             ,LL_SWI_FS_CLOSE              );
DECDEF_LLAPI_SWI(int,           llapi_fs_sync,               (fs_obj_t fobj)                             ,LL_SWI_FS_SYNC               );
DECDEF_LLAPI_SWI(int,           llapi_fs_read,               (fs_obj_t fobj, void* buf, uint32_t size)   ,LL_SWI_FS_READ               );
DECDEF_LLAPI_SWI(int,           llapi_fs_write,              (fs_obj_t fobj, void* buf, uint32_t size)   ,LL_SWI_FS_WRITE              );
DECDEF_LLAPI_SWI(int,           llapi_fs_seek,               (fs_obj_t fobj, uint32_t off, int whence)   ,LL_SWI_FS_SEEK               );
DECDEF_LLAPI_SWI(int,           llapi_fs_rewind,             (fs_obj_t fobj)                             ,LL_SWI_FS_REWIND             );
DECDEF_LLAPI_SWI(int,           llapi_fs_truncate,           (fs_obj_t fobj, uint32_t size)              ,LL_SWI_FS_TRUNCATE           );
DECDEF_LLAPI_SWI(uint32_t,      llapi_fs_tell,               (fs_obj_t fobj)                             ,LL_SWI_FS_TELL               );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_mkdir,          (const char* path)                          ,LL_SWI_FS_DIR_MKDIR          );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_open,           (fs_dir_obj_t dir_obj, const char* path)    ,LL_SWI_FS_DIR_OPEN           );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_close,          (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_CLOSE          );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_seek,           (fs_dir_obj_t dir_obj, uint32_t off)        ,LL_SWI_FS_DIR_SEEK           );
DECDEF_LLAPI_SWI(uint32_t,      llapi_fs_dir_tell,           (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_TELL           );
DECDEF_LLAPI_SWI(uint32_t,      llapi_fs_dir_rewind,         (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_REWIND         );
DECDEF_LLAPI_SWI(uint32_t,      llapi_fs_dir_read,           (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_READ           );
DECDEF_LLAPI_SWI(const char *,  llapi_fs_dir_cur_item_name,  (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_GET_CUR_NAME   );
DECDEF_LLAPI_SWI(uint32_t,      llapi_fs_dir_cur_item_size,  (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_GET_CUR_SIZE   );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_cur_item_type,  (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_GET_CUR_TYPE   );


#ifdef __cplusplus          
    }          
#endif


