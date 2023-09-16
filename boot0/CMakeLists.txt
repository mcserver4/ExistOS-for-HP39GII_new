cmake_minimum_required(VERSION 3.16)

include(ExternalProject)

# project(ExistOS_BOOT0 VERSION 0.1.0)




include_directories(${CMAKE_SOURCE_DIR}/boot0/webdata) 
include_directories(${CMAKE_SOURCE_DIR}/library/TinyUSB/lwip) 
include_directories(${CMAKE_SOURCE_DIR}/library/TinyUSB/lwip/include) 
aux_source_directory(${CMAKE_SOURCE_DIR}/library/TinyUSB/lwip/core                LWIP_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/library/TinyUSB/lwip/netif               LWIP_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/library/TinyUSB/lwip/arch                LWIP_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/library/TinyUSB/lwip/api                 LWIP_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/library/TinyUSB/lwip/core/ipv4           LWIP_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/library/TinyUSB/lwip/core/ipv6           LWIP_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/library/TinyUSB/lwip/apps/http           LWIP_SRCS)
list(REMOVE_ITEM   LWIP_SRCS    ${CMAKE_SOURCE_DIR}/library/TinyUSB/lwip/apps/http/fsdata_custom.c)
list(REMOVE_ITEM   LWIP_SRCS    ${CMAKE_SOURCE_DIR}/library/TinyUSB/lwip/apps/http/fsdata.c)
ADD_LIBRARY(lwip ${LWIP_SRCS})

include_directories(${CMAKE_SOURCE_DIR}/library/TinyUSB/src) 
include_directories(${CMAKE_SOURCE_DIR}/library/TinyUSB/src/common   ) 
include_directories(${CMAKE_SOURCE_DIR}/library/TinyUSB/src/device   )
include_directories(${CMAKE_SOURCE_DIR}/library/TinyUSB/src/portable/chipidea/ci_hs)
include_directories(${CMAKE_SOURCE_DIR}/library/TinyUSB/src/class/net)
include_directories(${CMAKE_SOURCE_DIR}/library/TinyUSB/networking)
aux_source_directory(${CMAKE_SOURCE_DIR}/library/TinyUSB/src                          TINYUSB_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/library/TinyUSB/src/common                   TINYUSB_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/library/TinyUSB/src/class/net                TINYUSB_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/library/TinyUSB/src/portable/chipidea/ci_hs  TINYUSB_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/library/TinyUSB/networking                   TINYUSB_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/library/TinyUSB/src/device                   TINYUSB_SRCS)
ADD_LIBRARY(tinyusb ${TINYUSB_SRCS})

include_directories(.) 
include_directories(./bsp/registers)  
include_directories(./bsp) 
aux_source_directory(.                   LOADER_SRCS)
aux_source_directory(./bsp               LOADER_SRCS) 
add_executable(boot0.elf ${LOADER_SRCS})

target_link_libraries(boot0.elf PRIVATE 
lwip tinyusb lfs  #coremark
-lnosys -lc_nano -nostartfiles  
-T ${CMAKE_SOURCE_DIR}/boot0/script/ld.lds -specs=nano.specs  ) #-specs=picolibc.specs -Wl,--gc-sections

target_compile_options(boot0.elf PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -marm -Wall -Os -specs=nano.specs)
target_compile_options(lwip PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -mthumb -Os  -specs=nano.specs)
target_compile_options(lfs PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -mthumb -Os  -specs=nano.specs)
target_compile_options(tinyusb PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft  -marm -Os  -specs=nano.specs)
  
add_custom_command(TARGET boot0.elf POST_BUILD
    COMMAND arm-none-eabi-size ${CMAKE_CURRENT_BINARY_DIR}/boot0.elf 
    )

add_custom_target(
  boot0.bin ALL
  COMMAND ${CMAKE_OBJCOPY} -I elf32-little -O binary -R .note -R .comment -S ${CMAKE_CURRENT_BINARY_DIR}/boot0.elf ${CMAKE_CURRENT_BINARY_DIR}/boot0.bin 
  DEPENDS boot0.elf 
  )

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")

    add_custom_target(
      boot0.sb ALL
      COMMAND ${CMAKE_SOURCE_DIR}/tools/sbtools_win/elftosb -z -c ${CMAKE_SOURCE_DIR}/boot0/script/build_fw.bd -o ${CMAKE_CURRENT_BINARY_DIR}/boot0.sb ${CMAKE_CURRENT_BINARY_DIR}/boot0.bin
      DEPENDS boot0.bin
      )

    add_custom_target(
      upload_boot0 
      COMMAND ${CMAKE_SOURCE_DIR}/tools/sbtools_win/sb_loader -f ${CMAKE_CURRENT_BINARY_DIR}/boot0.sb 
      DEPENDS boot0.sb
      )

else()
      if(EXISTS ${CMAKE_SOURCE_DIR}/tools/sbtools/elftosb)
          add_custom_target(
            boot0.sb ALL
            COMMAND ${CMAKE_SOURCE_DIR}/tools/sbtools/elftosb -z -c ${CMAKE_SOURCE_DIR}/boot0/script/build_fw.bd -o ${CMAKE_CURRENT_BINARY_DIR}/boot0.sb ${CMAKE_CURRENT_BINARY_DIR}/boot0.bin
            DEPENDS boot0.bin
            )
          add_custom_target(
            upload_boot0 
            COMMAND ${CMAKE_SOURCE_DIR}/tools/sbtools/sbloader ${CMAKE_CURRENT_BINARY_DIR}/boot0.sb -d
            DEPENDS boot0.sb
            )
      else(EXISTS ${CMAKE_SOURCE_DIR}/tools/sbtools/elftosb)
          string(ASCII 27 Esc)
          set(R "${Esc}[1;31;47m")
          set(E "${Esc}[0m")
          message(FATAL_ERROR "${R}!!!! Please build tools/sbtools first !!!! ${E}")
      endif(EXISTS ${CMAKE_SOURCE_DIR}/tools/sbtools/elftosb)
endif()
