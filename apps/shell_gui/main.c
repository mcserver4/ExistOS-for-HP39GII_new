
#include <stdio.h>
#include <stdlib.h>

#include "lvgl.h"

#include "llapi.h"
#include "framework/ui.h"
#include "keys_39gii.h"

#include "lvfs.h"
#include "lv_100ask_file_explorer.h"

#define LCD_WIDTH 256
#define LCD_HEIGHT 127



static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    int32_t x;
    int32_t y;
    for (y = area->y1; y <= area->y2; y++) {
        for (x = area->x1; x <= area->x2; x++) {
            /*Put a pixel to the display. For example:*/
            /*put_px(x, y, *color_p)*/
            uint8_t c = color_p->full;
            llapi_disp_put_point(x, y, c);
            color_p++;
        }
    }

    lv_disp_flush_ready(disp_drv);
}

static int16_t dx, dy, enc;
static bool touched;

static uint8_t scan_key()
{

    uint8_t key = llapi_query_key();
    //if(key != 0xFF)
    { 
        if(CHECK_KEY_PRESS(key, KEY_RIGHT))dx = 3;
        else if(CHECK_KEY_PRESS(key, KEY_LEFT))dx = -3;
        else if(CHECK_KEY_PRESS(key, KEY_DOWN))dy = 3;
        else if(CHECK_KEY_PRESS(key, KEY_UP))dy = -3;
        else if(CHECK_KEY_PRESS(key, KEY_F3))enc = -3;
        else if(CHECK_KEY_PRESS(key, KEY_F4))enc = 3;
        else if(/*CHECK_KEY_PRESS(key, KEY_ENTER) ||*/ CHECK_KEY_PRESS(key, KEY_F1))touched = true;

        else if(CHECK_KEY_RELEASE(key, KEY_RIGHT))dx = 0;
        else if(CHECK_KEY_RELEASE(key, KEY_LEFT))dx = 0;
        else if(CHECK_KEY_RELEASE(key, KEY_UP))dy = 0;
        else if(CHECK_KEY_RELEASE(key, KEY_DOWN))dy = 0;
        else if(CHECK_KEY_RELEASE(key, KEY_F3))enc = 0;
        else if(CHECK_KEY_RELEASE(key, KEY_F4))enc = 0;
        else if(/*CHECK_KEY_RELEASE(key, KEY_ENTER) ||*/ CHECK_KEY_RELEASE(key, KEY_F1))touched = false;
    }
    return key;
}

/*Read the touchpad*/
void mouse_read( lv_indev_drv_t * indev_drv, lv_indev_data_t * data )
{
    static int16_t touchX, touchY;
    //scan_key();

    touchX += dx;
    touchY += dy;
    if(touchX > 255)touchX = 255;
    if(touchY > 126)touchY = 126;
    if(touchX < 0)touchX = 0;
    if(touchY < 0)touchY = 0;


    /*Set the coordinates*/
    data->point.x = touchX;
    data->point.y = touchY;
    data->enc_diff = enc; 

    if( !touched )
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;
    }
}

static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static uint8_t last_key = 0;

    /*Get whether the a key is pressed and save the pressed key*/
    uint8_t raw_key = scan_key()  ;
    uint8_t act_key = raw_key & 0x7F;
    uint32_t press = !(raw_key >> 7);
    if(raw_key != 0xFF) {
        data->state = press ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL; 
        /*Translate the keys to LVGL control characters according to your key definitions*/
        switch(act_key) {
            case KEY_F3: 
                act_key =  LV_KEY_UP;
                break;
            case KEY_F4: 
                act_key =  LV_KEY_DOWN;
                break;
            case KEY_ENTER: 
                act_key =  LV_KEY_ENTER;
                break;
            default:
                act_key = 0; 
                data->state = LV_INDEV_STATE_REL;
                break;
        }
        last_key = act_key;
    }
    data->key = last_key;
}

int main() {

    lv_init();
    
    static lv_disp_draw_buf_t draw_buf_dsc_1;
    static lv_color_t buf_1[LCD_WIDTH * 10];
    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, LCD_WIDTH * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf_dsc_1;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = mouse_read;
    lv_indev_t *mouse = lv_indev_drv_register( &indev_drv );

    static lv_indev_drv_t indev_drv_keypad; 
    lv_indev_drv_init(&indev_drv_keypad);
    indev_drv_keypad.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv_keypad.read_cb = keypad_read;
    lv_indev_t *indev_keypad = lv_indev_drv_register(&indev_drv_keypad);
    lv_group_t * group = lv_group_create();
    lv_group_set_default(group);
    lv_indev_set_group(indev_keypad, group);


    
    LV_IMG_DECLARE(mouse_cursor_icon); 
    lv_obj_t * cursor_obj =  lv_img_create(lv_scr_act()); //create object
    lv_img_set_src(cursor_obj, &mouse_cursor_icon); //set its image
    lv_indev_set_cursor(mouse, cursor_obj); // connect the object to the driver

 
    lv_port_fs_init();
    ui_init();

     
    lv_label_set_text(ui_BtnF1Label, LV_SYMBOL_GPS);
    lv_label_set_text(ui_BtnF2Label, "");
    lv_label_set_text(ui_BtnF3Label, LV_SYMBOL_UP);
    lv_label_set_text(ui_BtnF4Label, LV_SYMBOL_DOWN);
    lv_label_set_text(ui_BtnF5Label, "");
    lv_label_set_text(ui_BtnF6Label, "");

    lv_obj_t *fe = lv_100ask_file_explorer_create(ui_TabPage1);
    lv_100ask_file_explorer_open_dir(fe,"A:/");


    printf("Start Loop...\r\n");
    while (1) {
        lv_timer_handler();
        llapi_delay_ms(10);
    }
    return 0;
}
