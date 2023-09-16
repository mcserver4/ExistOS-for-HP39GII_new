#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>

#include "loader.h"
#include "utils.h"

static void *heap = NULL;

uint32_t get_free_mem()
{
    return(HEAP_END - (uint32_t)heap);
}

caddr_t _sbrk(int incr)
{
    void *prev_heap;

    // print_uart0("_sbrk\r\n");
    if (heap == NULL)
    {
        // print_uart0("Set heap\r\n");
        heap = &__HEAP_START;
    }
    prev_heap = heap;
    if (((uint32_t)heap + incr) > HEAP_END)
    {
        print_uart0("MEMOF!\r\n");
        while (1)
            ;
        // printf("heap:%p, incr:%d\n", heap, incr);
        // return 0;
    }
    heap += incr;

    return (caddr_t)prev_heap;
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

#include "dhserver.h"
#include "dnserver.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/ethip6.h"

//extern struct udp_pcb *udppcb;

#define INIT_IP4(a, b, c, d)               \
    {                                      \
        PP_HTONL(LWIP_MAKEU32(a, b, c, d)) \
    }

_ssize_t _write_r(struct _reent *pReent, int fd, const void *buf, size_t nbytes)
{

    size_t i = 0;
    //struct pbuf *out;
    //out = pbuf_alloc(PBUF_TRANSPORT, nbytes, PBUF_POOL);

    //static const ip4_addr_t ipaddr = INIT_IP4(192, 168, 77, 2);

    while (nbytes--)
    {
        //if (udppcb)
        {
            //memcpy(out->payload, buf, nbytes);
            //udp_sendto(udppcb, out, &ipaddr, 57332);
            //pbuf_free(out);
        }
        print_putc(((char *)buf)[i]);
        i++;
    }
    return i;
}
