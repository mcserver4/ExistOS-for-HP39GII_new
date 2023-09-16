#include "FreeRTOS.h"
#include "task.h"
#include "hypercall.h"

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>

#include <utils.h>
#include <malloc.h>

/*
uint32_t vPortHeapTrimFreePage();

void *__wrap__malloc_r(struct _reent *pReent, size_t nbytes)
{
    return pvPortMalloc(nbytes);
}

void __wrap__free_r(struct _reent *pReent, void *p)
{
    vPortFree(p);
    xPortHeapTrimFreePage();
}

void *__wrap__calloc_r (struct _reent *r, size_t a, size_t b)
{
    print_uart0("calloc?\r\n");
    return pvPortCalloc(a, b);
}

void *__wrap__realloc_r(struct _reent *r, void *x, size_t sz)
{
    print_uart0("realloc?\r\n");
    void *p = pvPortMalloc(sz);
    if(!p)
    {
        return NULL;
    }
    memcpy(x, p, sz);
    vPortFree(x);
    return p;
}
 
 
caddr_t _sbrk(int incr)
{
    print_uart0("SYS C LIB MEMOF!\r\n");
    return 0; 
} 
*/

uint32_t putDec(int n)
{
        if(n < 0)
        {
            print_putc('-');
            n = -n;
        }
    while(n)
    {
        print_putc('0' + (n%10));
        n /= 10;
    }
}


extern int heap_start;
extern int heap_end;

uint8_t *heap = (uint8_t *)&heap_start; 
caddr_t _sbrk(int incr)
{ 
    //if(incr < 0)
    //{
    //    print_uart0("sbrk dec\r\n");
    //}
    //print_uart0("sbrk:");
    //putDec(incr);
    //print_uart0("\r\n");

    caddr_t cur = heap;
    if((uint32_t)heap + incr > (uint32_t)&heap_end)
    {
        print_uart0("SYS C LIB MEMOF!\r\n");
        return 0; 
    }else{ 
        heap += incr;
        return cur;
    }
} 

size_t xPortGetFreeHeapSize( void )
{
    return  (uint32_t)&heap_end  -  (uint32_t)heap;
}


void __malloc_lock (struct _reent *pReent)
{
    vTaskSuspendAll();
}
void __malloc_unlock (struct _reent *pReent)
{
    xTaskResumeAll();
}

void __free_lock (struct _reent *pReent)
{
    vTaskSuspendAll();
}
void __free_unlock (struct _reent *pReent)
{
    xTaskResumeAll();
}

static uint8_t mm_page_bitmap[520 / 8];
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

void *__real__malloc_r(struct _reent *pReent, size_t nbytes);
void __real__free_r(struct _reent *pReent, void *p);

void *__wrap__malloc_r(struct _reent *pReent, size_t nbytes)
{
    vTaskSuspendAll();
    if(mm_page_bitmap[0] != 0x5A)
    {
        memset(mm_page_bitmap, 0, sizeof(mm_page_bitmap));
        mm_page_bitmap[0] = 0x5A;
    }
    void *p = __real__malloc_r(pReent, nbytes);
    if(p)
    {
        uint32_t addr = (uint32_t)p & 0x000FFFFF;
        int pages = nbytes / 1024;
        //if(pages < 0)
        //{
        //    pages = 0;
        //}
        uint32_t s_pg = ((uint32_t)addr - 12) / 1024;
        for(int i = s_pg - 1; i < s_pg + pages + 2; i++)
        {
            b_set(i);
        }
    }
    xTaskResumeAll();
    return p;
}

void __wrap__free_r(struct _reent *pReent, void *p)
{
    vTaskSuspendAll();
    if(p)
    {
        uint32_t addr = (uint32_t)p & 0x000FFFFF;
        uint32_t sz = ((uint32_t *)p)[-1];
        int pages =  (sz / 1024) - 2;
        if(pages < 0)
        {
            pages = 0;
        }
        uint32_t s_pg = 1 + ((uint32_t)addr) / 1024;
        for(int i = s_pg; i < s_pg + pages; i++)
        {
            b_clr(i);
        }
    }
    __real__free_r(pReent, p);
    xTaskResumeAll();
}

uint32_t xPortGetAllocatedPages()
{
    uint32_t cnt = 0;
    uint32_t start = (((uint32_t)&heap_start) & 0x000FFFFF) / 1024 + 1;
    uint32_t stop = (((uint32_t)&heap_end) & 0x000FFFFF) / 1024;
    for(int i = start; i < stop; i++)
    {
        if(b_get(i))
        { 
            cnt ++;
        }
    }
    return cnt;
}

uint32_t xPortGetFreePages()
{
    uint32_t cnt = 0;
    uint32_t start = (((uint32_t)&heap_start) & 0x000FFFFF) / 1024 + 1;
    uint32_t stop = (((uint32_t)&heap_end) & 0x000FFFFF) / 1024;
    for(int i = start; i < stop; i++)
    {
        if(!b_get(i))
        { 
            cnt ++;
        }
    }
    return cnt;
}

uint32_t xPortHeapTrimFreePage()
{
    uint32_t cnt = 0;
    uint32_t clr_start = (((uint32_t)&heap_start) & 0x000FFFFF) / 1024 + 1;
    uint32_t clr_stop = (((uint32_t)&heap_end) & 0x000FFFFF) / 1024;
    for(int i = clr_start; i < clr_stop; i++)
    {
        if(b_get(i) == 0)
        {  
            cnt += ll_mm_trim_vaddr((i * 1024) | 0x02000000);
        }
    }
    return cnt;
}

int _close_r(struct _reent *pReent, int fd)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int _execve_r(struct _reent *pReent, const char *filename, char *const *argv, char *const *envp)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int _fcntl_r(struct _reent *pReent, int fd, int cmd, int arg)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int _fork_r(struct _reent *pReent)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int _fstat_r(struct _reent *pReent, int file, struct stat *st)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int _getpid_r(struct _reent *pReent)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int _isatty_r(struct _reent *pReent, int file)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int _kill_r(struct _reent *pReent, int pid, int signal)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int _link_r(struct _reent *pReent, const char *oldfile, const char *newfile)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

_off_t _lseek_r(struct _reent *pReent, int file, _off_t offset, int whence)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int _mkdir_r(struct _reent *pReent, const char *pathname, int mode)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int _open_r(struct _reent *pReent, const char *file, int flags, int mode)
{
 
    pReent->_errno = ENOTSUP;
    return -1;
}

_ssize_t _read_r(struct _reent *pReent, int fd, void *ptr, size_t len)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

_ssize_t _rename_r(struct _reent *pReent, const char *oldname, const char *newname)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int _stat_r(struct _reent *pReent, const char *__restrict __path, struct stat *__restrict __sbuf)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

_CLOCK_T_ _times_r(struct _reent *pReent, struct tms *tbuf)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int _unlink_r(struct _reent *pReent, const char *filename)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int _wait_r(struct _reent *pReent, int *wstat)
{
    pReent->_errno = ENOTSUP;
    return -1;
}

int fsync(int __fd)
{

    return -1;
}

char *getcwd(char *buf, size_t size)
{

    return NULL;
}

void abort(void)
{
    // Abort called
    while (1)
        ;
}

void _exit(int i)
{

    while (1)
        ;
}

static uint8_t stdout_rb[1024];
static uint16_t rb_w = 0, rb_r = 0;

uint8_t stdout_rb_read()
{
    uint8_t val;
    if(rb_r == rb_w)
        return 0;
    val = stdout_rb[rb_r];
    rb_r++;
    if(rb_r >= sizeof(stdout_rb))
    {
        rb_r = 0;
    }
    return val;
}

_ssize_t _write_r(struct _reent *pReent, int fd, const void *buf, size_t nbytes)
{

    size_t i = 0;
    while (nbytes--)
    {
        stdout_rb[rb_w] = ((char *)buf)[i];
        print_putc(((char *)buf)[i]);
        rb_w++;
        i++;

        if(rb_w >= sizeof(stdout_rb))
        {
            rb_w = 0;
        }
    }
    return i;
}
