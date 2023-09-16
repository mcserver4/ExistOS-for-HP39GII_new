#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "utils.h"

static void *heap = NULL;

uint32_t get_free_mem()
{
    return(HEAP_END - (uint32_t)heap);
}

caddr_t _sbrk(int incr)
{
    void *prev_heap;
 
    if (heap == NULL)
    { 
        heap = &__HEAP_START;
    }
    prev_heap = heap;
    if (((uint32_t)heap + incr) > HEAP_END)
    {
        print_uart0("MEMOF!\r\n");
        while (1)
            ; 
    }
    heap += incr;

    return (caddr_t)prev_heap;
}


_CLOCK_T_ _times_r(struct _reent *pReent, struct tms *tbuf)
{
    pReent->_errno = ENOTSUP;
    return -1;
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

int _wait_r(struct _reent *pReent, int *wstat)
{
    pReent->_errno = ENOTSUP;
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

 