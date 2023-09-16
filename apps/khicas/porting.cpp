#include "font.h"
#include "llapi.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#include "khicas_src/giac39/iostream_sub.h"
#include "khicas_src/giac39/giac.h"
#include "khicas_src/giac39/gen.h"
 #include "khicas_src/giac39/porting.h"

 // struct DISPBOX {
 //   int     left;
 //   int     top;
 //   int     right;
 //   int     bottom;
 //   unsigned char mode;
 // } ;

#define COLOR_CYAN   90
#define COLOR_RED    68
#define COLOR_GREEN  68
#define COLOR_WHITE  255
#define COLOR_BLACK  0

#define VIR_LCD_PIX_W  256
#define VIR_LCD_PIX_H  127


#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

void * __dso_handle = nullptr;
extern "C" int __cxa_atexit(void (*dtor)(void *), void * object, void * handle) {
  return 0;
}
extern "C" void __cxa_pure_virtual() {
  printf("Unsupported pure-virtual method.\n");
  while(1);
}


namespace giac{

unsigned int rtc_get_tick_ms()
{
    return llapi_get_tick_ms();
}

stdostream cout;

}

int kcas_main(int isAppli, unsigned short OptionNum);
extern "C" {

char screen_1bpp[256*128/8];

int khicas_1bpp = 1;
bool khicasRunning = false;
char keyStatus = 0;
int intBit = 0;
int rshift = 0; 
void (*XcasExitCb)(void) = NULL;
extern bool khicasRunning;
 

void vGL_FlushVScreen()
{
  if (khicas_1bpp){
    char *src=screen_1bpp;
    for (int r=0;r<VIR_LCD_PIX_H;++r){
      char tab[VIR_LCD_PIX_W];
      char *dest=tab,*end=dest+VIR_LCD_PIX_W;
      for (;dest<end;dest+=8,++src){
        char cur=*src;
        if (cur){
          dest[0]=(cur&1)?0xff:0; 
          dest[1]=(cur&2)?0xff:0; 
          dest[2]=(cur&4)?0xff:0; 
          dest[3]=(cur&8)?0xff:0; 
          dest[4]=(cur&16)?0xff:0; 
          dest[5]=(cur&32)?0xff:0; 
          dest[6]=(cur&64)?0xff:0; 
          dest[7]=(cur&128)?0xff:0; 
        }
        else {
          *((unsigned *) dest)=0;
          *((unsigned *) &dest[4])=0;
        }
      }
      llapi_disp_put_hline(r, tab);   
    }
    return;
    }
}


void vGL_set_pixel(unsigned x,unsigned y,int c) {
  //if (y==93) printf("glsetp:%d,%d,%d\n",x,y,c);
    if((x >= VIR_LCD_PIX_W ) || (y >= VIR_LCD_PIX_H))return;

  if (khicas_1bpp){
    char shift = 1<<(x&7);
    int pos=(x+VIR_LCD_PIX_W*y)>>3;
    if (c)
      screen_1bpp[pos] |= shift;
    else
      screen_1bpp[pos] &= ~shift;
  }
  else
    llapi_disp_put_point(x,y,c);
}

void vGL_SetPoint(unsigned int x, unsigned int y, int c)
{
 
    vGL_set_pixel(x,y,c);
} 

int vGL_GetPoint(unsigned int x,unsigned int y)
{
    if((x >= VIR_LCD_PIX_W ) || (y >= VIR_LCD_PIX_H))return 0;
    
    if (khicas_1bpp){
      char shift = 1<<(x&7);
      int pos=(x+VIR_LCD_PIX_W*y)>>3;
      return (screen_1bpp[pos] & shift)?0xff:0;
    }    
    return llapi_disp_get_point(x, y);
}  

void vGL_putChar(int x0, int y0, char ch, int fg, int bg, int fontSize) {
    int font_w;
    int font_h;
    const unsigned char *pCh;
    unsigned int x = 0, y = 0, i = 0, j = 0; 
 
    if ((ch < ' ') || (ch > '~' + 1)) {
        return;
    } 

    switch (fontSize) {
    case 8:
        font_w = 8;
        font_h = 8;
        pCh = VGA_Ascii_5x8 + (ch - ' ') * font_h;
        break;

    case 12:
        font_w = 8;
        font_h = 12;
        pCh = orp_Ascii_6x12 + (ch - ' ') * font_h;
        break;

    case 16:
        font_w = 8;
        font_h = 16;
        pCh = VGA_Ascii_8x16 + (ch - ' ') * font_h;
        break;

    case 14:
        font_w = 7;
        font_h = 14;
        pCh = VGA_Ascii_7x14 + (ch - ' ') * font_h;
        break;

    default:
        return;
    }

    while (y < font_h) {
        while (x < font_w) { 
              vGL_SetPoint(x0+x,y0+y, ((*pCh << x) & 0x80U)?fg:bg);
            x++;
        }
        x = 0;
        y++;
        pCh++;
    }
}


void vGL_putString(int x0, int y0, const char *s, int fg, int bg, int fontSize) {
    int font_w;
    int font_h;
    int len = strlen(s);
    int x = 0, y = 0;

    if (fontSize <= 16) {
        switch (fontSize) {
        case 8:
            font_w = 5;
            break;
        case 12:
            font_w = 6;
            break;
        case 14:
            font_w = 7;
            break;
        case 16:
            font_w = 8;
            break;
        default:
            font_w = 8;
            break;
        }

        font_h = fontSize;
        while (*s) {
            vGL_putChar((x0 * 1) + x, (y0 * 1) + y, *s, fg, bg, fontSize);
            s++;
            x += font_w;
            if (x > VIR_LCD_PIX_W) { 
                break;
                x = 0;
                y += font_h;
                //if (y > VIR_LCD_PIX_H) 
                //{
                //    break;
                //}
            }
        }
    }

    vGL_FlushVScreen();
}


void vGL_clearArea(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1) {
 

    printf("clra:%d,%d,%d,%d\n", x0, x1, y0, y1);
    for (int y = y0; y < y1; y++){
      for (int x = x0; x < x1; x++) {
        vGL_SetPoint(x,y,COLOR_WHITE); // virtual_screen[x + y * VIR_LCD_PIX_W] = COLOR_WHITE;
      }
    }
    vGL_FlushVScreen();
}


void vGL_setArea(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, unsigned int color) 
{
 
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            {
              vGL_SetPoint(x,y,color); // virtual_screen[x + y * VIR_LCD_PIX_W] = color;
            }
        }
    vGL_FlushVScreen();    
}


void vGL_reverseArea(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1) {
    
    if(  (x0 >= VIR_LCD_PIX_W )  || (y0 >= VIR_LCD_PIX_H))return;
    if(  (x1 >= VIR_LCD_PIX_W )  || (y1 >= VIR_LCD_PIX_H))return;

    if (khicas_1bpp){
      for (int y = y0; y < y1; y++){
        for (int x = x0; x < x1; x++) {
          char shift = 1<<(x&7);
          int pos=(x+VIR_LCD_PIX_W*y)>>3;
          screen_1bpp[pos] ^= shift; 
        }
      }
        vGL_FlushVScreen();
    }
    else
     {
      for (int y = y0; y < y1; y++){
        for (int x = x0; x < x1; x++) {
            //llapi_disp_put_point(x,y, ~llapi_disp_get_point(x,y));
             llapi_disp_put_point(x,y, ~(llapi_disp_get_point(x,y) & 0XFF));
        }
      }
    }
}



extern int shell_fontw, shell_fonth;
static bool cursor_enable = false;
static bool concur_reverse = false;
static int cursor_x = 0;
static int cursor_y = 0;

static void vGL_concur_reverse()
{
   concur_reverse = !concur_reverse;
   vGL_reverseArea((cursor_x + 1) * shell_fontw, (cursor_y + 1) * shell_fonth, (cursor_x + 1) * shell_fontw + 1, (cursor_y + 1) * shell_fonth + shell_fonth);
}

void vGL_locateConcur(int x, int y)
{
    if(concur_reverse)
    {
        vGL_concur_reverse();
    }
    cursor_x = x;
    cursor_y = y;
}

void vGL_concurEnable(bool enable)
{
    
    if(!enable && (cursor_enable))
    {
        vGL_concur_reverse();
    }
    cursor_enable = enable;
}


void flush_indBit()
{

}

#define INPUT_TRANSLATE(_rawin, normal, shift_l, shift_r,  alpha_capital, alpha_small) \
    case _rawin :    \
        {   \
          if(keyStatus & 1){*key = shift_l; keyStatus=0; flush_indBit(); } \
          else if(rshift)  {*key = shift_r;}  \
          else if(keyStatus & 4)  {*key = alpha_small; if(!(keyStatus&0x80)) keyStatus=0; flush_indBit();} \
          else if(keyStatus & 8)  {*key = alpha_capital; if(!(keyStatus&0x80)) keyStatus=0; flush_indBit();}  \
          else {*key = normal;}  \
          break;\
        }  \


bool IsKeyDown(int test_key)
{


    return false;
}

#include "keys_39gii.h"
#include "khicas_src/giac39/k_defs.h"
#define KEYREP_NOEVENT              0
#define KEYREP_KEYEVENT             1
#define KEYREP_TIMEREVENT           2

int  GetKey(int *key)
{
    uint8_t raw_key = llapi_query_key();
    int pkey = raw_key & 0x7F;
    bool press = !(raw_key >> 7);

    if (pkey != 0x7F && press) {
        // printf("pkey:%08x\n", key);

        switch (pkey) {


        case KEY_SHIFT:
            *key = -1;
            // *key = KEY_CTRL_SHIFT; 
            if (keyStatus & 1) {
                keyStatus &= ~1;
                rshift = 1;
            } else if(rshift)
            {
                rshift = 0;
            } else {
                keyStatus |= 1;
                rshift = 0;
            }

            //flush_indBit();

            break;

        case KEY_ALPHA:
            *key = -1;
            // *key = KEY_CTRL_ALPHA;

            if (keyStatus & 4) {
                keyStatus &= ~4;
                keyStatus |= 8;
            } else {

                if (keyStatus & 8) {
                    keyStatus &= ~4;
                    keyStatus &= ~8;
                } else {

                    keyStatus |= 4;
                }
            }

            //flush_indBit();

            break;

        
        INPUT_TRANSLATE(KEY_F1, KEY_CTRL_F1, KEY_CTRL_F1, KEY_CTRL_F1, KEY_CTRL_F1, KEY_CTRL_F1);
        INPUT_TRANSLATE(KEY_F2, KEY_CTRL_F2, KEY_CTRL_F2, KEY_CTRL_F2, KEY_CTRL_F2, KEY_CTRL_F2);
        INPUT_TRANSLATE(KEY_F3, KEY_CTRL_F3, KEY_CTRL_F3, KEY_CTRL_F3, KEY_CTRL_F3, KEY_CTRL_F3);
        INPUT_TRANSLATE(KEY_F4, KEY_CTRL_F4, KEY_CTRL_F4, KEY_CTRL_F4, KEY_CTRL_F4, KEY_CTRL_F4);
        INPUT_TRANSLATE(KEY_F5, KEY_CTRL_F5, KEY_CTRL_F5, KEY_CTRL_F5, KEY_CTRL_F5, KEY_CTRL_F5);
        INPUT_TRANSLATE(KEY_F6, KEY_CTRL_F6, KEY_CTRL_F6, KEY_CTRL_F6, KEY_CTRL_F6, KEY_CTRL_F6);
        
        //INPUT_TRANSLATE(_rawin, normal, shift_l, shift_r, alpha_small, alpha_capital)
        INPUT_TRANSLATE(KEY_UP, KEY_CTRL_UP, KEY_CTRL_PAGEUP, KEY_CTRL_UP, KEY_CTRL_UP, KEY_CTRL_UP);
        INPUT_TRANSLATE(KEY_DOWN, KEY_CTRL_DOWN, KEY_CTRL_PAGEDOWN, KEY_CTRL_DOWN, KEY_CTRL_DOWN, KEY_CTRL_DOWN);
        INPUT_TRANSLATE(KEY_LEFT, KEY_CTRL_LEFT, KEY_SHIFT_LEFT, KEY_SHIFT_LEFT, KEY_CTRL_LEFT, KEY_CTRL_LEFT);
        INPUT_TRANSLATE(KEY_RIGHT, KEY_CTRL_RIGHT, KEY_SHIFT_RIGHT, KEY_SHIFT_RIGHT, KEY_CTRL_RIGHT, KEY_CTRL_RIGHT);

        INPUT_TRANSLATE(KEY_VIEWS, KEY_CTRL_EXIT, KEY_CTRL_QUIT, KEY_CTRL_EXIT, KEY_CTRL_EXIT, KEY_CTRL_EXIT);
        INPUT_TRANSLATE(KEY_HOME, KEY_CTRL_MENU,KEY_CTRL_SETUP,KEY_CTRL_SETUP,KEY_CTRL_MENU,KEY_CTRL_MENU);

        INPUT_TRANSLATE(KEY_NUM, KEY_CTRL_OPTN, KEY_SHIFT_OPTN, KEY_SHIFT_OPTN, KEY_CTRL_OPTN, KEY_CTRL_OPTN);
        INPUT_TRANSLATE(KEY_APPS, KEY_CTRL_SD, KEY_BOOK, KEY_BOOK, KEY_BOOK, KEY_BOOK);
        INPUT_TRANSLATE(KEY_SYMB, KEY_CTRL_SYMB, KEY_CTRL_SETUP, KEY_CTRL_SETUP, KEY_CTRL_SETUP, KEY_CTRL_SETUP);
        INPUT_TRANSLATE(KEY_PLOT, KEY_CTRL_OK, KEY_CTRL_SETUP, KEY_CTRL_SETUP, KEY_CTRL_SETUP, KEY_CTRL_SETUP);
        INPUT_TRANSLATE(KEY_VARS, KEY_CTRL_VARS, KEY_CTRL_INS, KEY_CTRL_INS, 'a', 'A');
        INPUT_TRANSLATE(KEY_MATH, KEY_CTRL_CATALOG, KEY_CTRL_CATALOG, KEY_CTRL_CATALOG, 'b', 'B');
        INPUT_TRANSLATE(KEY_ABC, KEY_CTRL_FRACCNVRT, KEY_CTRL_MIXEDFRAC, 0000000000000, 'c', 'C');
        INPUT_TRANSLATE(KEY_XTPHIN, KEY_CTRL_XTT, KEY_CHAR_EXPN10, KEY_CHAR_EXPN10, 'd', 'D');
        INPUT_TRANSLATE(KEY_BACKSPACE, KEY_CTRL_DEL, KEY_CTRL_DEL, KEY_CTRL_DEL, KEY_CTRL_DEL, KEY_CTRL_DEL);
        INPUT_TRANSLATE(KEY_SIN, KEY_CHAR_SIN, KEY_CHAR_ASIN, KEY_CHAR_ASIN, 'e', 'E');
        INPUT_TRANSLATE(KEY_COS, KEY_CHAR_COS, KEY_CHAR_ACOS, KEY_CHAR_ACOS, 'f', 'F');
        INPUT_TRANSLATE(KEY_TAN, KEY_CHAR_TAN, KEY_CHAR_ATAN, KEY_CHAR_ATAN, 'g', 'G');
        INPUT_TRANSLATE(KEY_LN, KEY_CHAR_LN, KEY_CHAR_EXP, KEY_CHAR_EXP, 'h', 'H');
        INPUT_TRANSLATE(KEY_LOG, KEY_CHAR_LOG, KEY_CHAR_EXPN10, KEY_CHAR_EXPN10, 'i', 'I');
        INPUT_TRANSLATE(KEY_X2, KEY_CHAR_SQUARE, KEY_CHAR_ROOT, KEY_CHAR_ROOT, 'j', 'J');
        INPUT_TRANSLATE(KEY_XY, KEY_CHAR_POW, KEY_CHAR_POWROOT, KEY_CHAR_POWROOT, 'k', 'K');
 
        INPUT_TRANSLATE(KEY_LEFTBRACKET, '(', KEY_CTRL_CLIP, KEY_CTRL_CLIP, 'l', 'L');
        INPUT_TRANSLATE(KEY_RIGHTBRACET, ')', KEY_CTRL_PASTE, KEY_CTRL_PASTE, 'm', 'M');

        INPUT_TRANSLATE(KEY_COMMA, ',', ',', ',', 'o', 'O');

        INPUT_TRANSLATE(KEY_0, '0', KEY_CTRL_CATALOG, KEY_CTRL_CATALOG, '"', '"');
        INPUT_TRANSLATE(KEY_1, '1', KEY_CTRL_PRGM, KEY_CTRL_PRGM, 'x', 'X');
        INPUT_TRANSLATE(KEY_2, '2', KEY_CHAR_IMGNRY, KEY_CHAR_IMGNRY, 'y', 'Y');
        INPUT_TRANSLATE(KEY_3, '3', KEY_CHAR_PI, KEY_CHAR_PI, 'z', 'Z');
        INPUT_TRANSLATE(KEY_4, '4', KEY_CHAR_MAT, KEY_CHAR_MAT, 't', 'T');
        INPUT_TRANSLATE(KEY_5, '5', '[', '[', 'u', 'U');
        INPUT_TRANSLATE(KEY_6, '6', ']', ']', 'v', 'V');
        INPUT_TRANSLATE(KEY_7, '7', KEY_CHAR_LIST, KEY_CHAR_LIST, 'p', 'P');
        INPUT_TRANSLATE(KEY_8, '8', '{', '{', 'q', 'Q');
        INPUT_TRANSLATE(KEY_9, '9', '}', '}', 'r', 'R');

        INPUT_TRANSLATE(KEY_DOT, '.', '=', '`', ':', ':');


        INPUT_TRANSLATE(KEY_PLUS, KEY_CHAR_PLUS, KEY_CHAR_PLUS, KEY_CHAR_PLUS, ' ', ' ');
        INPUT_TRANSLATE(KEY_SUBTRACTION, KEY_CHAR_MINUS, KEY_CHAR_THETA, KEY_CHAR_ANGLE, 'w', 'W');
        INPUT_TRANSLATE(KEY_MULTIPLICATION, KEY_CHAR_MULT, '!', '!', 's', 'S');
        INPUT_TRANSLATE(KEY_DIVISION, KEY_CHAR_DIV, KEY_CHAR_RECIP, KEY_CHAR_PMINUS, 'n', 'N');

        //INPUT_TRANSLATE(KEY_ON, KEY_CTRL_AC, KEY_CTRL_QUIT, KEY_CTRL_AC, KEY_CTRL_AC, KEY_CTRL_AC);

        INPUT_TRANSLATE(KEY_ENTER, KEY_CTRL_EXE, KEY_CHAR_ANS, KEY_CHAR_ANS, KEY_CHAR_CR, KEY_CHAR_CR);

        INPUT_TRANSLATE(KEY_NEGATIVE, KEY_CHAR_PMINUS, '|', '|', ';', ';');

        case KEY_ON:
        {

            if(keyStatus & 1){
                if(XcasExitCb)
                {
                    
                    
                    vGL_clearArea(0, 0, 255, 126);
                    vGL_putString(0, 0, (char *)"Quitting...", COLOR_BLACK , COLOR_WHITE, 16);
                    vGL_putString(0, 16, (char *)"Waiting session save...", COLOR_BLACK , COLOR_WHITE, 16);

                    XcasExitCb();
                    
                    
                    //ll_flash_sync();
                    //printf("power OFF\n");
                    khicasRunning = false;
                    keyStatus = 0;
                    
                    llapi_delay_ms(100);
 
 
                }
            }  
            else if(rshift)  {*key = KEY_CTRL_AC;}  
            else if(keyStatus & 4)  {*key = KEY_CTRL_AC;}  
            else if(keyStatus & 8)  {*key = KEY_CTRL_AC;}  
            else {*key = KEY_CTRL_AC;}  

          break;

        }

        default:
            *key = -1;
            return KEYREP_NOEVENT;
            break;
        }

        return KEYREP_KEYEVENT;
    }

    *key = -1;

    return KEYREP_NOEVENT;
}


char Setup_GetEntry(unsigned int index) {
    // 0x14
    //  = 1 shift
    //  = 4 alpha 1
    //  = 8 alpha 2
    // printf("Setup_GetEntry:%d\n", index);

    if (index == 0x14) {

        return keyStatus;
    }
    printf("Setup_GetEntry:%d\n", index);
    return 0;
}

char *Setup_SetEntry(unsigned int index, char setting) {

    if (index == 0x14) {
        keyStatus = setting;
        //flush_indBit();
        return NULL;
    }
    // printf("Setup_SetEntry:%d, %d\n", index, setting);
    return NULL;
}


int RTC_GetTicks() {
    return llapi_rtc_get_s();
}

void RTC_SetDateTime(unsigned char *time) {
    uint8_t hours = time[4];
    uint8_t minus = time[5];
    printf("set time %02d:%02d\n", hours, minus);
    llapi_rtc_set_s (hours * (60 * 60) + minus * 60);
}

void Sleep(int millisecond) {
    llapi_delay_ms(millisecond);
}

//void sprint_double(char *ch, double d)
//{
//    sprintf(ch, "%g", d);
//}

int PrintMini(int x, int y, unsigned char const *s, int rev) {
    printf("Pmini(%d,%d,%d):%s\n", x,y,rev ,s);
    if (rev) {
        vGL_putString(x, y, (char *)s, 255, 0, 14);
    } else {
        vGL_putString(x, y, (char *)s, 0, 255, 14);
    }
    x += strlen((const char *) s)*7;
    return x;
}

int PrintXY(int x, int y, unsigned char const *s, int rev) {
      printf("Pxy(%d,%d,%d):%s\n",x,y, rev,s);
    if (rev) {
        vGL_putString(x, y, (char *)s, 255, 0, 16);
    } else {
        vGL_putString(x, y, (char *)s, 0, 255, 16);
    }
    x += strlen((const char *)s)*8;
    return x;
}





int Cursor_SetPosition(char column, char row) {
    //printf("CSP:%d:%d\n", column, row);
    vGL_locateConcur(column, row);
    return 0;
}
int Cursor_SetFlashStyle(short int flashstyle) {
    //printf("CSF:%d\n", flashstyle);
    vGL_concurEnable(true);
    return 0;
}

void Cursor_SetFlashMode(long flashmode) {
    
    //vGL_concurEnable(flashmode > 0);
}
void Cursor_SetFlashOff(void) {
    vGL_concurEnable(false);
}
void Cursor_SetFlashOn(char flash_style) {
    vGL_concurEnable(true);
}

int MB_ElementCount(const char *buf) // like strlen but for the graphical length of multibyte strings
{
    // printf("MB_ElementCount:%08x\n",buf);
    return strlen(buf); // FIXME for UTF8
}

void SetQuitHandler(void (*callback)(void)) {
    XcasExitCb = callback;
}

extern bool khicasRunning;
void start_khicas() {

    int ret = llapi_fs_dir_mkdir("xcas");
    printf("mkdir:%d\r\n",ret);
    khicasRunning = true;
    kcas_main(0, 0);
}


} //-------------extern "C"----------------------------------------------
 void PopUpWin(int win) {
    printf("PopUpWin:%d\n", win);
} 

void Bdisp_AreaClr_VRAM(DISPBOX *area) {
    //printf("clra:%d,%d,%d,%d\n", area->left, area->top, area->right, area->bottom);
    vGL_clearArea(area->left, area->top, area->right, area->bottom);
    // vGL_FlushVScreen();     
}  

void Bdisp_AreaFillVRAM(DISPBOX *area, unsigned short color) 
{
    //printf("SETAREA:%d,%d,%d,%d, %08x\n", area->left, area->top, area->right, area->bottom, color);
    vGL_setArea(area->left, area->top, area->right, area->bottom, color);
}

void Bdisp_AreaReverseVRAM(int x0, int y0, int x1, int y1) {
    //printf("areaRev:%d,%d,%d,%d\n", x0,y0,x1,y1);
    vGL_reverseArea(x0, y0, x1, y1); 

    // vGL_FlushVScreen();
}

void Bdisp_AllClr_VRAM() {
    vGL_clearArea(0, 0, VIR_LCD_PIX_W - 1, VIR_LCD_PIX_H - 1);
    // vGL_FlushVScreen();
}
void Bdisp_SetPoint_VRAM(int x, int y, int c) {
    //printf("setp:%d,%d,%d\n",x,y,c);
    //vGL_SetPoint(x, y, (c > 0) ? COLOR_BLACK : COLOR_WHITE);
    vGL_SetPoint(x, y, c);
}

int Bdisp_GetPoint_VRAM(int x, int y) {

    return vGL_GetPoint(x, y);
}

void Bdisp_PutDisp_DD() {
    // vGL_FlushVScreen();
    // printf("Bdisp_PutDisp_DD\n");
} 
 

void Bdisp_DrawLineVRAM(int x0, int y0, int x1, int y1) {
    printf("pline:%d,%d,%d,%d\n", x0, y0, x1, y1);
    
} 





#define MAIN_DIR "xcas"
#define RAW_PATH "\\\\fls0"

void Bfile_StrToName_ncpy(unsigned short *dest, const unsigned char *source, int len) {
    //int l = path_replace((char *)source);
    //memcpy(dest, path_buf, l);

    char *d = (char *)dest, *s = (char *)source;

    if(memcmp(RAW_PATH, s, sizeof(RAW_PATH) - 1) == 0)
    {   
        int ll = strlen(s);
        ll -= sizeof(RAW_PATH) + 1;
        strcpy(d, MAIN_DIR);

        d += sizeof(MAIN_DIR) - 1;
        s += sizeof(RAW_PATH) - 1;
        strcpy( d, 
                s);  
    }else{
        strcpy(d, MAIN_DIR "/");
        d += sizeof(MAIN_DIR "/") - 1;
        strcpy( d, 
                s);  
    } 

    for(int i = 0; i < len; i++)
    {
        if(d[i] == '\\')
        {
            d[i] = '/';
        }
    }


    printf("StrToName:%s->%s\n",(char *)source, (char *)dest);

    //char *cDest = (char *)dest;
    //strlcpy(cDest, (const char *)source, len);
}
void Bfile_NameToStr_ncpy(unsigned char *dest, const unsigned short *source , int len) {
    printf("NameToStr:%s\n", (char *)source);
    strlcpy((char *)dest, (const char *)source, len);
}


extern "C" bool file_exists(const char * filename);

bool file_exists(const char * filename){
  unsigned short dest[strlen(filename)+1]={0};
  Bfile_StrToName_ncpy(dest,(const unsigned char *)filename,strlen(filename));
  //FIL handle;
  //FRESULT fr = f_open(&handle, (char *)dest,(FA_OPEN_EXISTING | FA_READ | FA_WRITE));
  //if (fr != FR_OK)
  //  return false;
  char *m = (char *)malloc(llapi_fs_get_fobj_sz());

  int ret = llapi_fs_open(m, (char*)dest, O_RDONLY);
  printf("f_open:[%s] %d\n",(char *) dest,ret);
  
  if(ret)
  {
    free(m);
    return false;
  }
    
  llapi_fs_close(m);
  free(m);
  return true;
}

 

int Bfile_OpenFile_OS(unsigned short *pFile, int mode) {
    //path_replace((char *)pFile);
    char *c_path = (char *)pFile;
    printf("open[%d]:%s\n", mode, (char *)c_path);

    
    fs_obj_t handle = (fs_obj_t)malloc(llapi_fs_get_fobj_sz());
    int fr;
    if(handle == NULL)
    {
        return -1;
    }
    fr = llapi_fs_open(handle, (const char *)c_path, mode == _OPENMODE_READ ? O_RDONLY : O_RDWR  ); 
    printf("open res:%d\n",fr);
    if(fr)
    {
        printf("f_open:[%s]:err:%d,%d\n",c_path,fr,mode);
        free(handle);
        return -1;
    }
    return (int)handle;
}



int Bfile_CreateFile(unsigned short *pFile, int size) {
    
    char *c_path = (char *)pFile;
    printf("Create[%d]:%s\n", size, (char *)c_path);

    fs_obj_t handle = (fs_obj_t)malloc(llapi_fs_get_fobj_sz());
    int fr;
    if(handle == NULL)
    {
        return -1;
    }
    fr = llapi_fs_open(handle, (const char *)c_path, O_RDWR | O_CREAT | O_TRUNC  ); 
    if(fr)
    {
        printf("Create 1:[%s]:err:%d,%d\n",c_path,fr,size);
        free(handle);
        return -1;
    }
    fr = llapi_fs_truncate(handle, size);
    if(fr)
    {
        printf("Create 2:[%s]:err:%d,%d\n",c_path,fr,size);
        llapi_fs_close(handle);
        free(handle);
        return -1;
    }
    llapi_fs_close(handle);
    free(handle);
    return 0;
}

void Bfile_CloseFile_OS(int hFile) {
    fs_obj_t handle = (fs_obj_t)hFile;
    llapi_fs_close(handle);
    free(handle);
}

void Bfile_WriteFile_OS(int hFile, const char *data, size_t len) {
    if((len == 0))
    {
        return;
    }
    fs_obj_t handle = (fs_obj_t)hFile;
    int fr;
    fr = llapi_fs_write(handle, (void *)data, len);
    if(fr < 0)
    {
        printf("WR FAIL.\n");
    }
    //printf("Bfile_WriteFile_OS\n");
}

void Bfile_DeleteEntry(unsigned short *pFile) {
    llapi_fs_remove((char *)pFile);
}

int Bfile_ReadFile_OS(int HANDLE, void *buf, int size, int readpos) {

    fs_obj_t handle = (fs_obj_t)HANDLE;
    uint32_t br;
    int fr;
    if(!handle)
    {
        printf("VOID POINTER!\n");
    }
    if(readpos >= 0)
        fr = llapi_fs_seek(handle, readpos, SEEK_SET);
    if(fr < 0){
        printf("f_lseek failed.\n");
        return -1;
    }

    fr =  llapi_fs_read(handle, buf, size);
    if(fr < 0){
        printf("f_read failed.\n");
        return -1;
    }
    br = fr;
    return br;
}

int Bfile_ReadFile(int HANDLE, void *buf, int size, int readpos) {
    return Bfile_ReadFile_OS(HANDLE, buf, size, readpos);
}

size_t Bfile_GetFileSize_OS(int hFile) {
    fs_obj_t handle = (fs_obj_t)hFile;
    llapi_fs_seek(handle, 0, SEEK_END);
    return llapi_fs_tell(handle);
}

int Bfile_FindFirst(const unsigned short *pathname, int *FindHandle, const unsigned short *foundfile, void *fileinfo) {

    printf("findFirst:%s\n", pathname);
    FILE_INFO *foundfi = (FILE_INFO *)fileinfo;

    fs_dir_obj_t dir = malloc(llapi_fs_get_dirobj_sz());
    assert(dir);
    int ret = llapi_fs_dir_open(dir, (const char *)pathname);
    if(ret < 0)
    {
        free(dir);
        *FindHandle = 0;
        return -1;
    }

    ret = llapi_fs_dir_read(dir);
    if(ret > 0)
    {
        foundfi->fsize = llapi_fs_dir_cur_item_size(dir);
        strcpy((char *)foundfile, llapi_fs_dir_cur_item_name(dir));
        *FindHandle = (int)dir;
        return 0;
    }

    llapi_fs_dir_close(dir);
    free(dir);
    return -1;
}


int Bfile_FindNext(int FindHandle, const unsigned short *foundfile, void *fileinfo) {

    if(!FindHandle)
    {
        return -1;
    }

    fs_dir_obj_t fh = (fs_dir_obj_t)FindHandle;
    int fr;
    FILE_INFO *foundfi = (FILE_INFO *)fileinfo;

    fr = llapi_fs_dir_read(fh);
    if(fr <= 0)
    {
        return -1;
    }
    foundfi->fsize = llapi_fs_dir_cur_item_size(fh);
    strcpy((char *)foundfile, llapi_fs_dir_cur_item_name(fh));
    return 0;
}

int Bfile_FindClose(int FindHandle) {
    if(!FindHandle)
    {
        return -1;
    }
    fs_dir_obj_t fh = (fs_dir_obj_t)FindHandle;
    llapi_fs_dir_close(fh);
    free(fh);
    return 0;
}


char memLoad_path_buf[270];
void *memory_load(char *adresse) {
    printf("memLoad:%s\n",adresse);
    Bfile_StrToName_ncpy((unsigned short *)memLoad_path_buf, (const unsigned char *)adresse, 270);

    
    fs_obj_t file = (fs_obj_t)malloc(llapi_fs_get_fobj_sz());
    assert(file);

    int fr;
    fr =  llapi_fs_open(file, memLoad_path_buf, O_RDONLY);
    if(fr)
    {
        free(file);
        return NULL;
    }

    int fSize = 0;
    llapi_fs_seek(file, 0, SEEK_END);
    fSize = llapi_fs_tell(file);
    llapi_fs_seek(file, 0, SEEK_SET);
 
    char *mem;
    if(fSize)
    {
        mem = (char *)malloc(fSize);
        assert(mem);
        fr = llapi_fs_read(file , mem, fSize);
        if(fr < 0)
        {
            llapi_fs_close(file);
            free(mem);
            free(file);
            return NULL;
        }
        llapi_fs_close(file);
        return mem;
    }
    return NULL;
}






