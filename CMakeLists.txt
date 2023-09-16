cmake_minimum_required(VERSION 3.16)

include(ExternalProject)

set(CMAKE_TOOLCHAIN_FILE toolchain.cmake)
project(ExistOS VERSION 0.2.0)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

#SET(BUILD_SHARED_LIBRARIES OFF)


include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/library/lfs)
aux_source_directory(${CMAKE_SOURCE_DIR}/library/lfs                   LFS_SRCS)
ADD_LIBRARY(lfs ${LFS_SRCS})

 
include_directories(${CMAKE_SOURCE_DIR}/library/xprintfc) 
aux_source_directory(${CMAKE_SOURCE_DIR}/library/xprintfc                   XPRINTFC_SRCS)
ADD_LIBRARY(xprintfc ${XPRINTFC_SRCS})


include_directories(${CMAKE_SOURCE_DIR}/library/umm_malloc-1.1.2/src) 
aux_source_directory(${CMAKE_SOURCE_DIR}/library/umm_malloc-1.1.2/src                   UMM_MALLOC_SRCS)
ADD_LIBRARY(umm ${UMM_MALLOC_SRCS})


add_subdirectory(boot0)
add_subdirectory(boot1)
add_subdirectory(system) 
add_subdirectory(apps) 