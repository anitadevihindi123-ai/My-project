cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")

# 1. हेडर फाइल (include) को खुद ढूंढने के लिए स्मार्ट हंटिंग
find_path(NCNN_INCLUDE_DIR 
    NAMES ncnn/net.h
    PATHS ${PROJECT_ROOT}/ncnn
    PATH_SUFFIXES include ${ANDROID_ABI}/include arm64-v8a/include armeabi-v7a/include
)

# 2. लाइब्रेरी (libncnn.a) को खुद ढूंढने के लिए स्मार्ट हंटिंग
find_library(NCNN_LIBRARY 
    NAMES ncnn
    PATHS ${PROJECT_ROOT}/ncnn
    PATH_SUFFIXES lib/${ANDROID_ABI} ${ANDROID_ABI}/lib lib arm64-v8a/lib armeabi-v7a/lib
)

# जो ढूंढा है, उसे यहाँ जोड़ दो
include_directories(${NCNN_INCLUDE_DIR})

add_library(ncnn STATIC IMPORTED)
set_target_properties(ncnn PROPERTIES IMPORTED_LOCATION ${NCNN_LIBRARY})

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
