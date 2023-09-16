#include <stdio.h>
#include <string.h>
 
#include "llapi.h"


  
 
int main()
{
    printf("Start App..\r\n"); 

    ((uint32_t *)0x12340000)[0] = 1;

    while (1)
    {
        printf("loop...\r\n");
        llapi_delay_ms(1000); 
    }

}

