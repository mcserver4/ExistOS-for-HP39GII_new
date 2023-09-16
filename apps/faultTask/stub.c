

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <malloc.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>

#include "llapi.h"

void llprint(const char *s) {
    while (*s != '\0') {
        llapi_putc(*s);
        s++;
    }
}

extern int _ram_start;
extern int __HEAP_START;
extern int _init;
uint32_t heap_end = 0;

uint8_t *heap = NULL;

caddr_t _sbrk(int incr) {

    if (heap == NULL) {
        heap = (uint8_t *)&__HEAP_START;
        heap_end = (uint32_t)&_ram_start + llapi_get_ram_size() - ((uint32_t *)&_init)[2];
    }

    caddr_t cur = heap;
    if ((uint32_t)heap + incr > (uint32_t)heap_end) {
        llprint("APP C LIB MEMOF!\r\n");
        return 0;
    } else {
        heap += incr;
        return cur;
    }
}


static uint8_t mm_page_bitmap[512 / 8];
inline static void b_set(int i)
{
    mm_page_bitmap[i / 8] |= (1 << (i % 8));
}
inline static void b_clr(int i)
{
    mm_page_bitmap[i / 8] &= ~(1 << (i % 8));
}
inline static uint8_t b_get(int i)
{
    return (mm_page_bitmap[i / 8] >> (i % 8)) & 1;
}

uint32_t mem_trim()
{
    uint32_t cnt = 0;
    uint32_t clr_start = (((uint32_t)&__HEAP_START) & 0x000FFFFF) / 1024 + 1;
    uint32_t clr_stop = (((uint32_t)heap_end) & 0x000FFFFF) / 1024;
    for(int i = clr_start; i < clr_stop; i++)
    {
        if(b_get(i) == 0)
        {  
            //cnt += ll_mm_trim_vaddr((i * 1024) | 0x02000000);
        }
    }
    return cnt;
}

void *__real__malloc_r(struct _reent *pReent, size_t nbytes);
void __real__free_r(struct _reent *pReent, void *p);

void *__wrap__malloc_r(struct _reent *pReent, size_t nbytes)
{
    if(mm_page_bitmap[0] != 0x5A)
    {
        memset(mm_page_bitmap, 0, sizeof(mm_page_bitmap));
        mm_page_bitmap[0] = 0x5A;
    }
    void *p = __real__malloc_r(pReent, nbytes);
    if(p)
    {
        uint32_t addr = ((uint32_t)p & 0x000FFFFF) - 0x00080000;
        int pages = nbytes / 1024;
        uint32_t s_pg = ((uint32_t)addr - 12) / 1024;
        for(int i = s_pg - 1; i < s_pg + pages + 2; i++)
        {
            b_set(i);
        }
    }
    return p;
}

void __wrap__free_r(struct _reent *pReent, void *p)
{
    if(p)
    {
        uint32_t addr = ((uint32_t)p & 0x000FFFFF) - 0x00080000;
        uint32_t sz = ((uint32_t *)p)[-1];
        int pages =  (sz / 1024) - 2;
        if(pages < 0)
        {
            pages = 0;
        }
        uint32_t s_pg = 1 + ((uint32_t)addr) / 1024;
        //printf("trim:%d pages:%d\r\n", s_pg, pages);
        for(int i = s_pg; i < s_pg + pages; i++)
        {
            b_clr(i);
        }
    }
    __real__free_r(pReent, p);
}



int _close_r(struct _reent *pReent, int fd) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int _execve_r(struct _reent *pReent, const char *filename, char *const *argv, char *const *envp) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int _fcntl_r(struct _reent *pReent, int fd, int cmd, int arg) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int _fork_r(struct _reent *pReent) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int _fstat_r(struct _reent *pReent, int file, struct stat *st) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int _getpid_r(struct _reent *pReent) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int _isatty_r(struct _reent *pReent, int file) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int _kill_r(struct _reent *pReent, int pid, int signal) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int _link_r(struct _reent *pReent, const char *oldfile, const char *newfile) {
    pReent->_errno = ENOTSUP;
    return -1;
}

_off_t _lseek_r(struct _reent *pReent, int file, _off_t offset, int whence) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int _mkdir_r(struct _reent *pReent, const char *pathname, int mode) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int _open_r(struct _reent *pReent, const char *file, int flags, int mode) {
    pReent->_errno = ENOTSUP;
    return -1;
}

_ssize_t _read_r(struct _reent *pReent, int fd, void *ptr, size_t len) {
    pReent->_errno = ENOTSUP;
    return -1;
}

_ssize_t _rename_r(struct _reent *pReent, const char *oldname, const char *newname) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int _stat_r(struct _reent *pReent, const char *__restrict __path, struct stat *__restrict __sbuf) {
    pReent->_errno = ENOTSUP;
    return -1;
}

_CLOCK_T_ _times_r(struct _reent *pReent, struct tms *tbuf) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int _unlink_r(struct _reent *pReent, const char *filename) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int _wait_r(struct _reent *pReent, int *wstat) {
    pReent->_errno = ENOTSUP;
    return -1;
}

int fsync(int __fd) {

    return -1;
}

char *getcwd(char *buf, size_t size) {

    return NULL;
}

void abort(void) {
    // Abort called
    while (1)
        ;
}

void _exit(int i) {

    while (1)
        ;
}

_ssize_t _write_r(struct _reent *pReent, int fd, const void *buf, size_t nbytes) {

    size_t i = 0;
    while (nbytes--) {
        llapi_putc(((char *)buf)[i]);
        i++;
    }
    return i;
}
