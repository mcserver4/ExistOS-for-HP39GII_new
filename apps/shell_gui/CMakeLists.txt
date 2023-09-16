
 
SET(APP_NAME  ShellGUI)
 
include_directories(.)
include_directories(..)

include_directories(./fe)
aux_source_directory(./fe ${APP_NAME}_SRCS)
aux_source_directory(./framework ${APP_NAME}_SRCS)
aux_source_directory(./framework/screens ${APP_NAME}_SRCS)
aux_source_directory(. ${APP_NAME}_SRCS)
 
include_directories(lvgl/src)
include_directories(lvgl/src/core)
include_directories(lvgl/src/draw)
include_directories(lvgl/src/draw/sw)
include_directories(lvgl/src/font)
include_directories(lvgl/src/hal)
include_directories(lvgl/src/misc)
include_directories(lvgl/src/widgets)
aux_source_directory(lvgl/src             LVGL_SRC)
aux_source_directory(lvgl/src/core        LVGL_SRC)
aux_source_directory(lvgl/src/draw        LVGL_SRC)
aux_source_directory(lvgl/src/draw/sw     LVGL_SRC)
aux_source_directory(lvgl/src/extra       LVGL_SRC)
aux_source_directory(lvgl/src/font        LVGL_SRC)
aux_source_directory(lvgl/src/hal         LVGL_SRC)
aux_source_directory(lvgl/src/misc        LVGL_SRC)
aux_source_directory(lvgl/src/widgets     LVGL_SRC)
aux_source_directory(lvgl/src/extra/layouts     LVGL_SRC)
aux_source_directory(lvgl/src/extra/layouts/flex     LVGL_SRC)
aux_source_directory(lvgl/src/extra/layouts/grid     LVGL_SRC)
aux_source_directory(lvgl/src/extra/libs     LVGL_SRC)
aux_source_directory(lvgl/src/extra/libs/bmp     LVGL_SRC)
aux_source_directory(lvgl/src/extra/libs/ffmpeg     LVGL_SRC)
aux_source_directory(lvgl/src/extra/libs/freetype     LVGL_SRC)
aux_source_directory(lvgl/src/extra/libs/fsdrv     LVGL_SRC)
aux_source_directory(lvgl/src/extra/libs/gif     LVGL_SRC)
aux_source_directory(lvgl/src/extra/libs/png     LVGL_SRC)
aux_source_directory(lvgl/src/extra/libs/qrcode     LVGL_SRC)
aux_source_directory(lvgl/src/extra/libs/rlottie     LVGL_SRC)
aux_source_directory(lvgl/src/extra/libs/sjpg     LVGL_SRC)
aux_source_directory(lvgl/src/extra/others     LVGL_SRC)
aux_source_directory(lvgl/src/extra/others/fragment     LVGL_SRC)
aux_source_directory(lvgl/src/extra/others/gridnav     LVGL_SRC)
aux_source_directory(lvgl/src/extra/others/ime     LVGL_SRC)
aux_source_directory(lvgl/src/extra/others/imgfont     LVGL_SRC)
aux_source_directory(lvgl/src/extra/others/monkey     LVGL_SRC)
aux_source_directory(lvgl/src/extra/others/msg     LVGL_SRC)
aux_source_directory(lvgl/src/extra/others/snapshot     LVGL_SRC)
aux_source_directory(lvgl/src/extra/themes     LVGL_SRC)
aux_source_directory(lvgl/src/extra/themes/basic     LVGL_SRC)
aux_source_directory(lvgl/src/extra/themes/default     LVGL_SRC)
aux_source_directory(lvgl/src/extra/themes/mono     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/animimg     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/calendar     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/chart     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/colorwheel     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/imgbtn     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/keyboard     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/led     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/list     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/menu     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/meter     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/msgbox     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/span     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/spinbox     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/spinner     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/tabview     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/tileview     LVGL_SRC)
aux_source_directory(lvgl/src/extra/widgets/win      LVGL_SRC)
 
include_directories(lvgl/demos)
include_directories(lvgl/demos/benchmark)
aux_source_directory(lvgl/demos/benchmark             LVGL_SRC)
aux_source_directory(lvgl/demos/benchmark/assets             LVGL_SRC)

add_library(lvgl  ${LVGL_SRC})


add_executable(${APP_NAME}.elf ../llapi.c ${${APP_NAME}_SRCS})
target_compile_options(${APP_NAME}.elf PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -marm -O3  )
target_compile_options(lvgl PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -marm -O3  )


target_link_libraries(${APP_NAME}.elf PRIVATE 
lvgl
-lnosys -lc -lm -lgcc -lstdc++ -nostartfiles  
-T ${CMAKE_SOURCE_DIR}/apps/script/ld.lds)

add_custom_command(TARGET ${APP_NAME}.elf POST_BUILD
    COMMAND arm-none-eabi-size ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.elf 
    )

add_custom_target(
  ${APP_NAME}.exp ALL
  COMMAND ${CMAKE_OBJCOPY} -I elf32-little -O binary -R .note -R .comment -S ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.elf ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.exp 
  DEPENDS ${APP_NAME}.elf 
  )
