cmake_minimum_required(VERSION 3.16)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_CXX_STANDARD 17)

set(CCE_CMAKE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)
list(APPEND CMAKE_MODULE_PATH ${CCE_CMAKE_PATH})
project(kernel_samples LANGUAGES CCE CXX)

if(ASCEND_PRODUCT_TYPE STREQUAL "Kirin9050")
    set(KERNEL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/add_custom_micro.cpp)
else()
  set(KERNEL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/add_custom.cpp)
endif()

add_subdirectory(cmake/npu)