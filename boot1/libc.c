#include "config.h"
#include "mm.h"
#include "lfs.h"
#include "utils.h"

 
#include <sys/errno.h>
#include <unistd.h>
#include <sys/fcntl.h>

#include "umm_malloc.h"
#include "xformatc.h"
#include "utils.h"

static char buf1[64];
static void myPutchar(void *arg,char c)
{
    char ** s = (char **)arg;
    *(*s)++ = c;
    print_putc(c);
}

static void myPrintf(char *buf,const char *fmt,va_list args)
{
    xvformat(myPutchar,(void *)&buf,fmt,args);
    *buf = 0;
}


void __wrap_printf(const char * fmt,...)
{

    va_list list;
    va_start(list,fmt);
    myPrintf(buf1,fmt,list);
    va_end(list);
}


#define LBLOCKSIZE (sizeof(long))
#define UNALIGNED(X)   ((long)X & (LBLOCKSIZE - 1))
#define TOO_SMALL(LEN) ((LEN) < LBLOCKSIZE)
void *memset (void *m, int c, size_t n)
{
  char *s = (char *) m;
#if !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__)
  int i;
  unsigned long buffer;
  unsigned long *aligned_addr;
  unsigned int d = c & 0xff;	/* To avoid sign extension, copy C to an
				   unsigned variable.  */
  while (UNALIGNED (s))
    {
      if (n--)
        *s++ = (char) c;
      else
        return m;
    }
  if (!TOO_SMALL (n))
    {
      /* If we get this far, we know that n is large and s is word-aligned. */
      aligned_addr = (unsigned long *) s;
      /* Store D into each char sized location in BUFFER so that
         we can set large blocks quickly.  */
      buffer = (d << 8) | d;
      buffer |= (buffer << 16);
      for (i = 32; i < LBLOCKSIZE * 8; i <<= 1)
        buffer = (buffer << i) | buffer;
      /* Unroll the loop.  */
      while (n >= LBLOCKSIZE*4)
        {
          *aligned_addr++ = buffer;
          *aligned_addr++ = buffer;
          *aligned_addr++ = buffer;
          *aligned_addr++ = buffer;
          n -= 4*LBLOCKSIZE;
        }
      while (n >= LBLOCKSIZE)
        {
          *aligned_addr++ = buffer;
          n -= LBLOCKSIZE;
        }
      /* Pick up the remainder with a bytewise loop.  */
      s = (char*)aligned_addr;
    }
#endif /* not PREFER_SIZE_OVER_SPEED */
  while (n--)
    *s++ = (char) c;
  return m;
}
/*
void * memset(void *p, int c, size_t sz)
{
    char *q = (char *)p;
    while(sz)
    {
        *q = c;
        q++;
        sz--;
    }
    return p;
}*/

int puts(const char *s)
{
    return print_uart0(s);
}

void *calloc(size_t s1, size_t s2)
{
    return umm_calloc(s1,s2);
}

void *malloc(size_t s1)
{
    return umm_malloc(s1);
}

void free(void *p)
{
    umm_free(p);
}

size_t strlen(const char *s)
{
	const char *sc;
	for (sc = s; *sc != '\0'; ++sc);
	return sc - s;
}

char *strcpy(char *dest, const char *src)
{
	char *tmp = dest;
	while ((*dest++ = *src++) != '\0');
	return tmp;
}

int putchar(int c)
{
    print_putc(c);
    return c;
}

void *memcpy(void *dest, const void *src, size_t count)
{
	char *tmp = dest;
	const char *s = src;

	while (count--)
		*tmp++ = *s++;
	return dest;
} 
 
size_t strspn(const char *s, const char *accept)
{
	const char *p;

	for (p = s; *p != '\0'; ++p) {
		if (!strchr(accept, *p))
			break;
	}
	return p - s;
} 
size_t strcspn(const char *s, const char *reject)
{
	const char *p;

	for (p = s; *p != '\0'; ++p) {
		if (strchr(reject, *p))
			break;
	}
	return p - s;
}

char *strchr(const char *s, int c)
{
	for (; *s != (char)c; ++s)
		if (*s == '\0')
			return NULL;
	return (char *)s;
}

int memcmp(const void *cs, const void *ct, size_t count)
{
	const unsigned char *su1, *su2;
	int res = 0;
	for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}
