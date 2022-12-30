#ifndef K_DEFS_H
#define K_DEFS_H
#define NSPIRE_FILEBUFFER 512*1024
  // Character codes
#define KEY_CHAR_0          0x30 /* 0 */
#define KEY_CHAR_1          0x31 /* 1 */
#define KEY_CHAR_2          0x32 /* 2 */
#define KEY_CHAR_3          0x33 /* 3 */
#define KEY_CHAR_4          0x34 /* 4 */
#define KEY_CHAR_5          0x35 /* 5 */
#define KEY_CHAR_6          0x36 /* 6 */
#define KEY_CHAR_7          0x37 /* 7 */
#define KEY_CHAR_8          0x38 /* 8 */
#define KEY_CHAR_9          0x39 /* 9 */
#define KEY_CHAR_DP         0x2e /* . */
#define KEY_CHAR_EXP        0x0f /* ee */
#define KEY_CHAR_PMINUS     30200 /* - */
#define KEY_CHAR_PLUS       43   /* + */
#define KEY_CHAR_MINUS      45   /* - */
#define KEY_CHAR_MULT       42   /* * */
#define KEY_CHAR_DIV        47   /* / */
#define KEY_CHAR_FRAC       0xbb /* KEY_CTRL_F14 */
#define KEY_CHAR_LPAR       0x28 /* ( */
#define KEY_CHAR_RPAR       0x29 /* ) */
#define KEY_CHAR_COMMA      0x2c /* , */
#define KEY_CHAR_STORE      0x0e /* => */
#define KEY_CHAR_LOG        0x95 /* log10( */
#define KEY_CHAR_LN         0x85 /* ln( */
#define KEY_CHAR_SIN        0x81 /* sin( */
#define KEY_CHAR_COS        0x82 /* cos( */
#define KEY_CHAR_TAN        0x83 /* tan( */
#define KEY_CHAR_SQUARE     0x8b /* ^2 */
#define KEY_CHAR_POW        0xa8 /* ^ */
#define KEY_CHAR_IMGNRY     0x7f50 /* i */
#define KEY_CHAR_LIST       0x7f51 /* KEY_CTRL_F9 */
#define KEY_CHAR_MAT        0x7f40 /* KEY_CTRL_F8 */
#define KEY_CHAR_EQUAL      0x3d /* = */
#define KEY_CHAR_PI         0xd0 /* pi */
#define KEY_CHAR_ANS        0xc0 /* ans() */
#define KEY_SHIFT_ANS       0xc1
#define KEY_CHAR_LBRCKT     0x5b /* [ */
#define KEY_CHAR_RBRCKT     0x5d /* ] */
#define KEY_CHAR_LBRACE     0x7b /* { */
#define KEY_CHAR_RBRACE     0x7d /* } */
#define KEY_CHAR_CR         0x0d
#define KEY_CHAR_CUBEROOT   0x96 /* ^(1/3) */
#define KEY_CHAR_RECIP      0x9b /* ^(-1) */
#define KEY_CHAR_ANGLE      0x7f54 /* KEY_CTRL_F13 */
#define KEY_CHAR_EXPN10     0xb5 /* 10^( */
#define KEY_CHAR_EXPN       0xa5 /* e^( */
#define KEY_CHAR_ASIN       0x91 /* asin( */
#define KEY_CHAR_ACOS       0x92 /* acos( */
#define KEY_CHAR_ATAN       0x93 /* atan( */
#define KEY_CHAR_ROOT       0x86 /* sqrt( */
#define KEY_CHAR_POWROOT    0xb8 /* ^(1/ */
#define KEY_CHAR_SPACE      0x20 /* ' ' */
#define KEY_CHAR_DQUATE     0x22 /* " */
#define KEY_CHAR_VALR       0xcd /* abs( */
#define KEY_CHAR_THETA      0xce /* arg( */
#define KEY_CHAR_FACTOR     0xda
#define KEY_CHAR_NORMAL     0xdb
#define KEY_CHAR_SHIFTMINUS 0xdc
#define KEY_CHAR_CROCHETS   0xdd
#define KEY_CHAR_ACCOLADES  0xde
#define KEY_CHAR_A          0x41
#define KEY_CHAR_B          0x42
#define KEY_CHAR_C          0x43
#define KEY_CHAR_D          0x44
#define KEY_CHAR_E          0x45
#define KEY_CHAR_F          0x46
#define KEY_CHAR_G          0x47
#define KEY_CHAR_H          0x48
#define KEY_CHAR_I          0x49
#define KEY_CHAR_J          0x4a
#define KEY_CHAR_K          0x4b
#define KEY_CHAR_L          0x4c
#define KEY_CHAR_M          0x4d
#define KEY_CHAR_N          0x4e
#define KEY_CHAR_O          0x4f
#define KEY_CHAR_P          0x50
#define KEY_CHAR_Q          0x51
#define KEY_CHAR_R          0x52
#define KEY_CHAR_S          0x53
#define KEY_CHAR_T          0x54
#define KEY_CHAR_U          0x55
#define KEY_CHAR_V          0x56
#define KEY_CHAR_W          0x57
#define KEY_CHAR_X          0x58
#define KEY_CHAR_Y          0x59
#define KEY_CHAR_Z          0x5a


  // Control codes
#define KEY_CTRL_NOP        30202
#define KEY_CTRL_EXE        30201
#define KEY_CTRL_DEL        30025
#define KEY_CTRL_AC         30070
#define KEY_CTRL_FD         30046
#define KEY_CTRL_UNDO       30045
#define KEY_CTRL_XTT        30001
#define KEY_CTRL_EXIT       5
#define KEY_CTRL_OK         4
#define KEY_CTRL_SHIFT      30006
#define KEY_CTRL_ALPHA      30007
#define KEY_CTRL_OPTN       30008
#define KEY_CTRL_VARS       30030
#define KEY_CTRL_UP         1
#define KEY_CTRL_DOWN       2
#define KEY_CTRL_LEFT       0
#define KEY_CTRL_RIGHT      3
#define KEY_CTRL_F1         30009
#define KEY_CTRL_F2         30010
#define KEY_CTRL_F3         30011
#define KEY_CTRL_F4         30012
#define KEY_CTRL_F5         30013
#define KEY_CTRL_F6         30014
#define KEY_CTRL_F7         30915
#define KEY_CTRL_F8         30916
#define KEY_CTRL_F9         30917
#define KEY_CTRL_F10        30918
#define KEY_CTRL_F11        30919
#define KEY_CTRL_F12        30920
#define KEY_CTRL_F13        30921
#define KEY_CTRL_F14        30922
#define KEY_CTRL_F15        30923
#define KEY_CTRL_F16        30924
#define KEY_CTRL_F17        30925
#define KEY_CTRL_F18        30926
#define KEY_CTRL_F19        30927
#define KEY_CTRL_F20        30928
#define KEY_CTRL_CATALOG    30100
#define KEY_CTRL_FORMAT     30203
#define KEY_CTRL_CAPTURE    30055
#define KEY_CTRL_CLIP       30050
#define KEY_CTRL_CUT        30250
#define KEY_CTRL_PASTE      30036
#define KEY_CTRL_INS        30033
#define KEY_CTRL_MIXEDFRAC  30054
#define KEY_CTRL_FRACCNVRT  30026
#define KEY_CTRL_QUIT       30029
#define KEY_CTRL_PRGM       30028
#define KEY_CTRL_SETUP      30037
#define KEY_CTRL_PAGEUP     30052
#define KEY_CTRL_PAGEDOWN   30053
#define KEY_CTRL_MENU       30003
#define KEY_SHIFT_OPTN	    30059
#define KEY_CTRL_RESERVE1	30060
#define KEY_CTRL_RESERVE2	30061
#define KEY_SHIFT_LEFT		30062
#define KEY_SHIFT_RIGHT		30063
#define KEY_UP_CTRL         31060
#define KEY_DOWN_CTRL       31061
#define KEY_LEFT_CTRL       31062
#define KEY_RIGHT_CTRL      31063
#define KEY_CALCULATOR      31064
#define KEY_SAVE            31065
#define KEY_LOAD            31066
#define KEY_CTRL_A          31001
#define KEY_CTRL_D          31004
#define KEY_CTRL_E          31005
#define KEY_CTRL_H          31008 // help?
#define KEY_CTRL_M          31011 // doc menu
#define KEY_CTRL_N          31012 
#define KEY_CTRL_R          31018
#define KEY_CTRL_S          31019
#define KEY_CTRL_T          31020
#define KEY_EQW_TEMPLATE    31100
#define KEY_AFFECT          31101
#define KEY_FLAG            31102
#define KEY_BOOK            31103
//#define KEY_CTRL_APPS     31104
#define KEY_CTRL_SYMB       31105
//#define KEY_CTRL_NUM      31106
//#define KEY_CTRL_PLOT     31107
#define KEY_SELECT_LEFT     31200
#define KEY_SELECT_UP       31201
#define KEY_SELECT_DOWN     31202
#define KEY_SELECT_RIGHT    31203
#define KEY_SHUTDOWN        32109

#define KEY_PRGM_ACON 10
#define KEY_PRGM_DOWN 37
#define KEY_PRGM_EXIT 47
#define KEY_PRGM_F1 79
#define KEY_PRGM_F2 69
#define KEY_PRGM_F3 59
#define KEY_PRGM_F4 49
#define KEY_PRGM_F5 39
#define KEY_PRGM_F6 29
#define KEY_PRGM_LEFT 38
#define KEY_PRGM_NONE 0
#define KEY_PRGM_RETURN 31
#define KEY_PRGM_RIGHT 27
#define KEY_PRGM_UP 28
#define KEY_PRGM_1 72
#define KEY_PRGM_2 62
#define KEY_PRGM_3 52
#define KEY_PRGM_4 73
#define KEY_PRGM_5 63
#define KEY_PRGM_6 53
#define KEY_PRGM_7 74
#define KEY_PRGM_8 64
#define KEY_PRGM_9 54
#define KEY_PRGM_A 76
#define KEY_PRGM_F 26
#define KEY_PRGM_ALPHA 77 
#define KEY_PRGM_SHIFT 78
#define KEY_PRGM_MENU 48
#define KEY_CTRL_SD         39990

#endif