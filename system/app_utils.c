#include "app_utils.h"
#include "app_runtime.h"

#include "../apps/llapi.h"
#include "queue.h"
#include <fcntl.h>

bool isUsingApp = false;
int appLen = 0;
ExpUtilsConfig_t *conf = NULL;

void StackInit(DIR_Element **tStack) {
    *tStack = (DIR_Element *)malloc(sizeof(DIR_Element));
    if(tStack==NULL)
    {
        printf("mallocError:StackInit\n");
        vTaskDelete(NULL);
    }
    (*tStack)->head = NULL;
    (*tStack)->length = 0;
}

void NodeAddition(DIR_Element *tStack, const char *filename, bool isDir, int size) {
    Node *newNode = malloc(sizeof(Node));
    if(newNode==NULL)
    {
        printf("mallocError:tStack::AddNode\n");
        vTaskDelete(NULL);
    }
    newNode->isDir = isDir;
    newNode->size = size;
    strcpy(newNode->name, filename);
    if (!tStack->head) {
        // no elem situation
        tStack->head = newNode;
        newNode->next = NULL;
        newNode->prev = NULL;
        tStack->length++;
        return;
    }
    newNode->next = tStack->head;
    tStack->head->prev = newNode;
    tStack->head = newNode;
    newNode->prev = NULL;
    tStack->length++;
}

bool NodeDelete(DIR_Element *tStack) {
    if (tStack->head) {
        Node *n = tStack->head;
        tStack->head = tStack->head->next;
        free(n);
        tStack->length--;
        return true;
    }
    return false;
}

void ExpkeyScanning();

void debug(char *content) {
    bsp_display_putk_string(0, 20, content, 0, 255);
    vTaskDelay(pdMS_TO_TICKS(500));
}

void get_app_name(char *path, char *app_name) {
    if (app_name == NULL)
        app_name = malloc(64 * sizeof(char));
    memset(app_name, 0, 64);
    int ptr;
    int lastSlash = 0;
    for (ptr = 0; *(path + ptr) != 0 && *(path + ptr) != '.'; ptr++)
        if (*(path + ptr) == '/' || *(path + ptr) == '\\')
            lastSlash = ptr;
    // cpyfileName
    for (int i = lastSlash + 1; i < ptr && i < 63; i++)
        *(app_name + i - lastSlash - 1) = *(path + i);
}

ExpUtilsConfig_t *getConfigFile() {
    fs_obj_t f = malloc(ll_fs_get_fobj_sz());
    if(f==NULL)
    {
        printf("mallocError:configFileGetter\n");
        return NULL;
    }
    int res = ll_fs_open(f, "/exp/config.dat", O_RDONLY);
    if (res) {
        //
        free(f);
        return NULL;
    }
    if (ll_fs_read(f, conf, sizeof(ExpUtilsConfig_t))) {
        // read err;
        ll_fs_close(f);
        free(f);
        return NULL;
    }

    ll_fs_close(f);
    free(f);
    return conf;
}

bool saveConfigFile(ExpUtilsConfig_t *conf) {
    fs_obj_t f = malloc(ll_fs_get_fobj_sz());
    if(f==NULL)
    {
        printf("mallocError:configFileGetter\n");
        return NULL;
    }
    int res = ll_fs_open(f, "/exp/config.dat", O_CREAT | O_WRONLY);
    if (res) {
        ll_fs_close(f);
        free(f);
        return false;
    }
    ll_fs_write(f, conf, sizeof(ExpUtilsConfig_t));
    ll_fs_close(f);
    free(f);
    return true;
}

void app_create_folder() {
    ll_fs_dir_mkdir("/expcache");
}

/*
AppInd *getApps() {
    if (!conf)
        return NULL;
    AppInd *ind = malloc(sizeof(AppInd));
    ind->len = 0;
    int startIndex = 0, endIndex = 0;
    for (endIndex = 0; *(conf->memoryAppPath + endIndex); endIndex++) {
        if (*(conf->memoryAppPath + endIndex) == ';') {
            // check app valid
            AppElement *ele = malloc(sizeof(AppElement));
            memcpy(ele->appFilePath, conf->memoryAppPath + startIndex, endIndex - startIndex);
            fs_obj_t *f = malloc(sizeof(fs_obj_t));
            if (ll_fs_open(f, ele->appFilePath, O_RDONLY)) {
                free(f);
                free(ele);
                continue;
            }
            // ext:read exp file
            char *name;
            get_app_name(ele->appFilePath, name);
            memcpy(ele->appName, name, sizeof(char) * 64);
            free(name);
            ele->fileSize = ll_fs_size(f);
            ll_fs_close(f);
            free(f);
            ind->app[ind->len++] = ele;
            startIndex = endIndex + 1;
        }
    }
    if (startIndex == endIndex) {
        return ind;
    }

    // check app valid
    AppElement *ele = malloc(sizeof(AppElement));
    memcpy(ele->appFilePath, conf->memoryAppPath + startIndex, endIndex - startIndex);
    fs_obj_t *f = malloc(sizeof(fs_obj_t));
    if (ll_fs_open(f, ele->appFilePath, O_RDONLY)) {
        free(f);
        free(ele);
        return ind;
    }
    // ext:read exp file
    char *name;
    get_app_name(ele->appFilePath, name);
    memcpy(ele->appName, name, sizeof(char) * 64);
    free(name);
    ele->fileSize = ll_fs_size(f);
    ll_fs_close(f);
    free(f);
    ind->app[ind->len++] = ele;
    return ind;
}
*/

void drawUIFrame(char (*workTableContent)[5]) {
    uint8_t *lineBuf = malloc(sizeof(uint8_t) * 256);
    if(lineBuf==NULL)
    {
        printf("mallocError:lineBufInUIFrame\n");
        vTaskDelete(NULL);
    }
    memset(lineBuf, 0, 256 * sizeof(uint8_t));
    bsp_diaplay_put_hline(110, lineBuf);
    bsp_diaplay_put_hline(18, lineBuf);
    memset(lineBuf, 255, 256 * sizeof(uint8_t));
    for (int i = 19; i < 109; i++) {
        bsp_diaplay_put_hline(i, lineBuf);
    }
    for (int i = 1; i <= 5; i++) {
        lineBuf[i * 42] = 0;
    }
    for (int i = 111; i < 127; i++) {
        bsp_diaplay_put_hline(i, lineBuf);
    }
    for (int i = 0; i < 6; i++) {
        bsp_display_putk_string(1 + 44 * i, 112, workTableContent[i], 0, 255);
    }
    free(lineBuf);
}

void refreshScreenY(int stY, int edY, uint8_t c) {
    uint8_t *lineBuf = malloc(sizeof(uint8_t) * 256);
    if(lineBuf==NULL)
    {
        printf("mallocError:lineBufInRefScrY\n");
        vTaskDelete(NULL);
    }
    memset(lineBuf, c, 256 * sizeof(uint8_t));
    for (int i = stY; i < edY; i++) {
        bsp_diaplay_put_hline(i, lineBuf);
    }
    free(lineBuf);
}

void drawBlank(int x, int y, int width, uint8_t c) {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < width; j++) {
            bsp_display_set_point(x + i - 24, y + j - 24, c);
        }
    }
}

bool judgeSuffix(char *fileName, char *requireSuffix) {
    if (requireSuffix == NULL)
        return true;
    int i;
    for (i = 0; *(fileName + i) != 0; i++) {
        if (*(fileName + i) == '.') {
            return !strcmp((fileName + i + 1), requireSuffix);
        }
    }
    return 0;
}

void enterDIR(char *path, char *dst) {
    strcat(path, "/");
    strcat(path, dst);
}

bool exitDIR(char *path) {
    int i;
    for (i = 0; *(path + i) != 0; i++)
        ; // findTail
    if (i <= 1)
        return false;
    for (i = 0; *(path + i) != '/'; *(path + i--) = 0)
        ;
    return true;
}

char appName[100];
void recover() {
    app_recover_from_status(appName);
    xTaskCreate(ExpkeyScanning, "ExpKeyScanning", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL);
    vTaskDelete(NULL);
}

void startApp() {
    printf("startAppPath:%s\n", conf->customFilePath);
    printf("pre start\n");
    app_pre_start(conf->customFilePath, false, 0);

    printf("pre start OK.start App");
    app_start();
    xTaskCreate(ExpkeyScanning, "ExpKeyScanning", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL);
    vTaskDelete(NULL);
}


void ExpUI() {
    char workTable[6][5] = {"<", "TASK", "^", "v", "DEL", "OPEN"};
    char workTable1[6][5] = {"<", "APP", "^", "v", "DEL", "OPEN"};
    int flag = 1, lastFlag = -1; // 0-task rec,1-file manager
    // if (app_is_running())
    // flag = 0;

    int ptrU = 0; // localUserPointer
    int ptr = 0;
    bool refresh = true;
    bool dirChanged = true;
    int stateFS = -1;

    // uint8_t *tmpBuffer = malloc(sizeof(uint8_t) * 64 * 64);
    char *fullPath = malloc(sizeof(char) * 512);
    if(fullPath==NULL)
    {
        printf("mallocError:fullPath\n");
        vTaskDelete(NULL);
    }
    memset(fullPath, 0, sizeof(char) * 512);

    char *filePath = malloc(sizeof(char) * 512);
    if(filePath==NULL)
    {
        printf("mallocError:filePath\n");
        vTaskDelete(NULL);
    }
    memset(filePath, 0, sizeof(char) * 512);
    // memset(tmpBuffer, 0, sizeof(uint8_t) * 64 * 64);
    // ExpStatus_t *exp = malloc(sizeof(ExpStatus_t));
    fs_dir_obj_t dirObj = malloc(ll_fs_get_dirobj_sz());
    if(dirObj==NULL)
    {
        printf("mallocError:dirObj\n");
        vTaskDelete(NULL);
    }
    DIR_Element *ele;
    StackInit(&ele);
    Node *head = NULL;

    for (;;) {
        if (refresh) {
            
            // task rec
            if(flag==0) {

                if (lastFlag != flag) {
                    // draw UIFrame
                    drawUIFrame(workTable1);
                    bsp_display_putk_string(1, 1, "TaskRec", 0, 255);
                    vTaskDelay(1);
                    strcpy(filePath, "/expcache");
                    ptr = ptrU = 0;
                    lastFlag = flag;
                    ptr = 0;
                    ll_fs_dir_rewind(dirObj);
                }
                refreshScreenY(19, 109, 255);

                // 73,88_16
                // 39,84_21
                //  refresh content
                drawBlank(73, 46 + 42 * ptrU, 16, 100);
                drawBlank(39, 42 + 42 * ptrU, 21, 100);

            } 
            // launcher EXP select
            else {
                if (lastFlag != flag) {
                    // draw UIFrame
                    drawUIFrame(workTable);
                    strcpy(filePath, "/");
                    lastFlag = flag;
                }

                if (dirChanged) {

                    while (NodeDelete(ele))
                        ;
                    stateFS = ll_fs_dir_open(dirObj, filePath);
                    int i = 0;
                    while (ll_fs_dir_read(dirObj) > 0) {
                        if (i < 2) {
                            i++;
                            continue;
                        }
                        NodeAddition(ele, ll_fs_dir_cur_item_name(dirObj), ll_fs_dir_cur_item_type(dirObj) == FS_FILE_TYPE_DIR, ll_fs_dir_cur_item_size(dirObj));
                    }
                    ll_fs_close(dirObj);
                    head = ele->head;
                }

                if (ptrU < 0) {
                    ptrU = 0;
                    if (head->prev != NULL) {
                        head = head->prev;
                    }
                }

                if (ptrU > ptr) {
                    ptrU = ptr;
                    if (head->next != NULL) {
                        head = head->next;
                    }
                }

                if (!stateFS) {
                    /*
                        strcpy(fullPath,ptr==ptrU?">>":" ");
                        if(ll_fs_dir_cur_item_type(dirObj)==FS_FILE_TYPE_DIR){
                            strcat(fullPath,"/");
                            strcat(fullPath,ll_fs_dir_cur_item_name(dirObj));
                            bsp_display_putk_string(1,20+ptr*17,fullPath,0,255);
                        }else{
                            strcat(fullPath,ll_fs_dir_cur_item_name(dirObj));
                            bsp_display_putk_string(1,20+ptr*17,fullPath,0,255);

                            sprintf(fullPath,"%5dB",ll_fs_dir_cur_item_size(dirObj));
                            bsp_display_putk_string(190,20+ptr*17,fullPath,0,255);
                        }
                    */
                    printf("ptr:%d ptrU:%d\n",ptr,ptrU);
                    Node *tmp = head;
                    if (tmp != NULL) {
                        for (ptr = 0; ptr < 5; ptr++) {
                            strcpy(fullPath, ptr == ptrU ? ">>" : " ");
                            if (tmp->isDir) {
                                strcat(fullPath, "/");
                                strcat(fullPath, tmp->name);
                                bsp_display_putk_string(1, 20 + ptr * 17, fullPath, 0, 255);
                            } else {
                                strcat(fullPath, tmp->name);
                                bsp_display_putk_string(1, 20 + ptr * 17, fullPath, 0, 255);

                                sprintf(fullPath, "%5dB", tmp->size);
                                bsp_display_putk_string(180, 20 + ptr * 17, fullPath, 0, 255);
                            }

                            if (tmp->next != NULL)
                                tmp = tmp->next;
                            else
                                break;
                        }
                        ptr--;
                    }
                } else {
                    bsp_display_putk_string(1, 64, "Err:Cannot Open Dir", 0, 255);
                }

            
            }
            refresh = false;
        }

        if (bsp_is_key_down(KEY_UP) || bsp_is_key_down(KEY_F3)) {
            ptrU--;
            refresh = true;
        }
        if (bsp_is_key_down(KEY_DOWN) || bsp_is_key_down(KEY_F4)) {
            ptrU++;
            refresh = true;
        }
        if (bsp_is_key_down(KEY_F2)) {
            // flag = !flag; // change mode
            refresh = true;
        }
        if (bsp_is_key_down(KEY_F1)) {
            if (flag)
                exitDIR(filePath);
            else {
                // back original tsk
            }
            refresh = true;
        }
        if (bsp_is_key_down(KEY_F5)) {
            // delete func
        }
        if (bsp_is_key_down(KEY_F6)) {
            if (flag) {
                if (true) {
                    enterDIR(filePath, "");
                } else {
                    if (judgeSuffix(fullPath, "exp") && conf != NULL) {
                        free(fullPath);
                        free(filePath);
                        // free(tmpBuffer);
                        // free(exp);
                        xTaskCreate(startApp, "startTask", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL);
                    }
                }
            } else {

                free(fullPath);
                free(filePath);
                // free(tmpBuffer);
                // free(exp);
                xTaskCreate(recover, "recTask", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL);
            }
            refresh = true;
        }
        // printf("%d\n",count);
    }
}

void test(){
    char ss[2];
        
        ss[0] = ' ';
        ss[1] = 0;
        for(int y = 0; y < 128/16; y++)
        {
            for(int x = 0; x < 256 / 8; x++)
            {
                bsp_display_putk_string(x*8, y*16, ss, 0, 255);
                ss[0]++;
                if(ss[0] > 'z')
                    ss[0] = ' ';
            }
        }
}

void test1(){
    static char ss[2];  
       ss[1] = 0;
       for(int y = 0; y < 128/16; y++)
       {
           for(int x = 0; x < 256 / 8; x++)
           {
               if(ss[0] > 'z')
                   ss[0] = ' ';
               bsp_display_putk_string(x*8, y*16, ss, 0, 255);
               ss[0]++;
           }
       }
}

void test2(){
    while(1)
    for(int y = 0; y < 128; y++)
        {
            srand(y);
            for(int x = 0; x < 256; x++)
            {
               bsp_display_set_point(x,y,rand());
            }
        }
}

// after sys init complete
void appUtilsInit() {
    app_create_folder();
    conf = getConfigFile();
    if (conf == NULL) {
        conf = malloc(sizeof(ExpUtilsConfig_t));
        memset(conf->customFilePath, 0, 1024 * sizeof(char));
        xTaskCreate(ExpUI, "ExpUI", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, NULL);

    } else {
        if (judgeSuffix(conf->customFilePath, "exp")) {
            xTaskCreate(startApp, "startTask", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL);
            xTaskCreate(ExpkeyScanning, "ExpkeyScanning", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL);
            saveConfigFile(conf);
        } else {
            xTaskCreate(ExpUI, "ExpUI", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, NULL);
        }
    }
}

void ExpkeyScanning() {
    printf("enterScanning\n");
    int counter = 0;
    for (;;) {
        if (bsp_is_key_down(KEY_ON))
            counter++;
        else
            counter = 0;

        if (counter >= 5) {
            app_stop();
            xTaskCreate(ExpUI, "ExpUI", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, NULL);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}