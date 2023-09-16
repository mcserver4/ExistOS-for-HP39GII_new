
#include <stdio.h>
#include <stdlib.h>
#include "coremark.h"
#include "llapi.h"


int coremain();



uint32_t _randSeed = 1;

uint32_t _rand(void)
{
    uint32_t value;
    //Use a linear congruential generator (LCG) to update the state of the PRNG
    _randSeed *= 1103515245;
    _randSeed += 12345;
    value = (_randSeed >> 16) & 0x07FF;
    _randSeed *= 1103515245;
    _randSeed += 12345;
    value <<= 10;
    value |= (_randSeed >> 16) & 0x03FF;
    _randSeed *= 1103515245;
    _randSeed += 12345;
    value <<= 10;
    value |= (_randSeed >> 16) & 0x03FF;
    //Return the random value
    return value;
}



void memTestWrite(uint32_t base, uint32_t length, uint32_t seed)
{
    uint32_t val;
    uint8_t *p = (uint8_t *)base;
    
    _randSeed = seed;
    printf("memtest start\r\n");

    for(int i = 0; i < length;i++)
    {
        val = _rand() & 0xFF;        
        if(i < 20)
        {
            printf("TST WR:%02lX\r\n", val);
        }
        p[i] = val;
    }
    
    printf("memtest write fin\r\n");
}

void memTestRead(uint32_t base, uint32_t length, bool trim, uint32_t seed)
{
    uint32_t val;
    uint8_t *p = (uint8_t *)base; 
    
    _randSeed = seed;
    for(int i = 0; i < length;i++)
    {
        val = _rand() & 0xFF;
        if(i < 20)
        {
            printf("TST RD:%02lX\r\n", val);
        }
        if(p[i] != val)
        {
            printf("MEM ERR:%p, %lX == %X\r\n",&p[i],val,p[i]);
        }
    }

    //if(trim)
        //for(int i = 0; i < length;i+=1024)
            //mm_trim_page((uint32_t)&p[i]); 
 

    printf("memtest fin\r\n");
}


int main()
{
    //uint8_t *tstm = calloc(1,320*1024);

    while(1)
    {
        //memTestWrite((uint32_t)tstm, 320*1024, 123);

        printf("APP Test\r\n");
        coremain();
        llapi_delay_ms(3000);
    }
    return 0;
}




