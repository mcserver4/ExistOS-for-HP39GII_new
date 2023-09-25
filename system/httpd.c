#include "existos.h"

#include "system/bsp/board.h"
#include "tusb.h"

#include "dhserver.h"
#include "dnserver.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"

#include "FreeRTOS.h"
#include "task.h"

#define READ_DATA_SCREEN     ((void *)(-1))
#define READ_STATUS          ((void *)(-2))
#define READ_STDOUT          ((void *)(-3))

extern unsigned char const bmpHead_256127[1078];
uint8_t stdout_rb_read();

bool sys_state_print = true;

#define CMP_CMD_PREFIX(x)   memcmp(name,(x),sizeof(x))==0

int fs_open_custom(struct fs_file *file, const char *name) {
    
    if(CMP_CMD_PREFIX("/sys_state_pr"))
    {
        int set = 0;
        sscanf(name, "/sys_state_pr_%d", &set);
        sys_state_print = set;
        file->index = 0;
        file->len = 0;
        INFO("sys_state_print:%d\r\n",set);
        return 1;
    }
    else if(strcmp(name,"/screen.bmp") == 0)
    {
        file->len = 1078 + 256*127 + 2;
        file->index = 0;
        file->pextension = READ_DATA_SCREEN;
        return 1;
    }
    else if(strcmp(name,"/getStdout") == 0)
    {
        file->len = 2048;
        file->index = 0;
        file->pextension = READ_STDOUT;
        return 1;
    }
    else if(strcmp(name,"/reset") == 0)
    {
        bsp_reset();
        return 1;
    }

    return 0;
}
int fs_read_custom(struct fs_file *file, char *buffer, int count) 
{
    //printf("count:%d, f:%d\r\n",count, file->data);
    if(file->pextension == READ_STDOUT)
    {
        uint8_t val;
        int i = 0;
        val = stdout_rb_read();
        if(!val)
            return -1;
        do{
            buffer[i++] = val;
            count--;
        }while((val = stdout_rb_read()) && count);
        return i;
    }
    else if(file->pextension == READ_DATA_SCREEN)
    {
        if(file->index < 1078)
        {
            if(count < 1078)
            {
                memcpy(buffer, &bmpHead_256127[file->index], count);
                file->index += count;
                return count;
            }
            else
            {
                memcpy(buffer, &bmpHead_256127[file->index], 1078);
                file->index += 1078;
                return 1078;
            }
        }else if(file->index < 1078 + 256*127){ 
            int lines = (count+255) / 256 - 1;
            int c = 0;
            vTaskSuspendAll();
            for(int j =0; j < lines; j++)
            {
                for(int i = 0; i < 256; i++)
                {
                    buffer[c] = bsp_display_get_point(i, (file->index - 1078) /256);
                    c++;
                    file->index++;
                    if( file->index >= 1078 + 256*127)
                        break;
                }
            }
            xTaskResumeAll();
            return c;
        }else{
            buffer[0] = buffer[1] = 0;
            file->index+=2;
            return 2;
        }
    }

    printf("rd oversize\r\n");
    return -1;
}

void fs_close_custom(struct fs_file *file) {

}

err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                       u16_t http_request_len, int content_len, char *response_uri,
                       u16_t response_uri_len, u8_t *post_auto_wnd)
{
    return ERR_OK;
}

err_t httpd_post_receive_data(void *connection, struct pbuf *p) {
    return ERR_OK;
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len) {

}


