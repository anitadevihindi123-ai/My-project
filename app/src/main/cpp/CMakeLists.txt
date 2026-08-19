cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")

# 1. NCNN का पाथ ठीक करना (app फोल्डर के हिसाब سے)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../ncnn/include)

# 2. NCNN की प्री-बिल्ड लाइब्रेरी का पाथ ठीक करना
add_library(ncnn STATIC IMPORTED)
set_target_properties(ncnn PROPERTIES IMPORTED_LOCATION
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ncnn/lib/${ANDROID_ABI}/libncnn.a)

add_library(
    true-singularity-core
    SHARED
    true-singularity-core.cpp
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(
    true-singularity-core
    ncnn
    ${log-lib}
    ${android-lib}
)
