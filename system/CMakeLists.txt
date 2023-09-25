
project(ExistOS)
 
include_directories(./FreeRTOS)
include_directories(./FreeRTOS/include)
include_directories(./FreeRTOS/portable)

aux_source_directory(./FreeRTOS            FREERTOS_SRCS )
aux_source_directory(./FreeRTOS/include    FREERTOS_SRCS )
aux_source_directory(./FreeRTOS/portable   FREERTOS_SRCS )

ADD_LIBRARY(freertos  ${FREERTOS_SRCS})

include_directories(${CMAKE_SOURCE_DIR})
include_directories(./)
include_directories(./bsp)
include_directories(./bsp/registers)
aux_source_directory(.             SYS_SRCS)
aux_source_directory(./bsp         SYS_SRCS)
add_executable(sys.elf      ${SYS_SRCS})



include_directories(${CMAKE_SOURCE_DIR}/system/webdata) 
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
ADD_LIBRARY(lwip_sys ${LWIP_SRCS})


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
ADD_LIBRARY(tinyusb_sys ${TINYUSB_SRCS})
     
 
target_link_libraries(sys.elf PRIVATE 
-Wl,--wrap=_malloc_r 
-Wl,--wrap=_free_r 
#-Wl,--wrap=printf 
#-Wl,--wrap=_printf_r
#-Wl,--wrap=_calloc_r
#-Wl,--wrap=_realloc_r
 lwip_sys tinyusb_sys freertos xprintfc
-lc -lm 
-nostartfiles  
-T ${CMAKE_SOURCE_DIR}/system/script/ld.lds)
 

target_compile_options(xprintfc PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -mthumb  -Og   )
target_compile_options(lwip_sys PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -marm  -Og   )
target_compile_options(tinyusb_sys PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -marm  -Og   )
target_compile_options(freertos PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -marm  -Og  )
target_compile_options(sys.elf PRIVATE -mtune=arm926ej-s -mcpu=arm926ej-s -mlittle-endian -mfloat-abi=soft -marm -Og  ) 

   
add_custom_command(TARGET sys.elf POST_BUILD
    COMMAND arm-none-eabi-size ${CMAKE_CURRENT_BINARY_DIR}/sys.elf 
    )

add_custom_target(
  sys.bin ALL
  COMMAND ${CMAKE_OBJCOPY} -I elf32-little -O binary -R .note -R .comment -S ${CMAKE_CURRENT_BINARY_DIR}/sys.elf ${CMAKE_CURRENT_BINARY_DIR}/sys.bin 
  DEPENDS sys.elf 
  )

