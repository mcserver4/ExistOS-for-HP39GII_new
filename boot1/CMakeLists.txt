cmake_minimum_required(VERSION 3.16)

include(ExternalProject) 

include_directories(.) 
include_directories(./bsp/registers)   
include_directories(./bsp) 
aux_source_directory(.                   BOOT1_SRCS)
aux_source_directory(./bsp               BOOT1_SRCS)  

 
include_directories(../boot0/lfs) 
add_executable(boot1_head.elf ${BOOT1_SRCS})
 

target_compile_options(umm PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -mthumb  -Os -specs=nano.specs -ffunction-sections -fdata-sections  )
target_compile_options(xprintfc PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -mthumb  -Os -specs=nano.specs -ffunction-sections -fdata-sections  )
target_compile_options(lfs PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -mthumb -Os  -specs=nano.specs -ffunction-sections -fdata-sections) #
target_compile_options(boot1_head.elf PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mthumb -Wall -Os -specs=nano.specs -ffunction-sections -fdata-sections) # 
 
 
target_link_libraries(boot1_head.elf PRIVATE 
-Wl,--wrap=printf  
lfs xprintfc umm
-nostdlib -lgcc  
  -nostartfiles 
-T ${CMAKE_SOURCE_DIR}/boot1/script/ld.lds -specs=nano.specs -Wl,--gc-sections)  #-Wl,--gc-sections
 

add_custom_command(TARGET boot1_head.elf POST_BUILD
    COMMAND arm-none-eabi-size ${CMAKE_CURRENT_BINARY_DIR}/boot1_head.elf 
    )

add_custom_target(
  boot1_head.bin ALL
  COMMAND ${CMAKE_OBJCOPY} -I elf32-little -O binary -R .note -R .comment -S ${CMAKE_CURRENT_BINARY_DIR}/boot1_head.elf ${CMAKE_CURRENT_BINARY_DIR}/boot1_head.bin 
  DEPENDS boot1_head.elf 
  )

