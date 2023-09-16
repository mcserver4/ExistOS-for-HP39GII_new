#include "existos.h"
#include "xformatc.h"
#include "utils.h"
static char buf1[512];

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

//int _printf_r(struct _reent * r, const char *__restrict fmt, ...)
//{
//
//}

void __wrap_printf(const char * fmt, ...)
{

    va_list list;
    va_start(list,fmt);
    myPrintf(buf1,fmt,list);
    va_end(list);
}


