
 
SET(APP_NAME  CoreMark)
 
 
include_directories(coremark)
aux_source_directory(coremark       CM_SRCS )
ADD_LIBRARY(coremark  ${CM_SRCS})


include_directories(.)
aux_source_directory(. ${APP_NAME}_SRCS)

include_directories(..)


add_executable(${APP_NAME}.elf ../llapi.c ${${APP_NAME}_SRCS})
target_compile_options(${APP_NAME}.elf PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -marm -Os  )
target_compile_options(coremark PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -marm  -O3   )

target_link_libraries(${APP_NAME}.elf PRIVATE 
coremark
-lnosys -lc -lm -lgcc -lstdc++ -nostartfiles  
-T ${CMAKE_SOURCE_DIR}/apps/script/ld.lds)

add_custom_command(TARGET ${APP_NAME}.elf POST_BUILD
    COMMAND arm-none-eabi-size ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.elf 
    )

add_custom_target(
  ${APP_NAME}.bin ALL
  COMMAND ${CMAKE_OBJCOPY} -I elf32-little -O binary -R .note -R .comment -S ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.elf ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.bin 
  DEPENDS ${APP_NAME}.elf 
  )
