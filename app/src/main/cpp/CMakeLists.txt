cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")

# प्रोजेक्ट का रूट पाथ सेट करते हैं ताकि कुत्ता भटके नहीं
get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}/../../../" ABSOLUTE)

# 1. हेडर फाइल (net.h) के लिए स्मार्ट हंटिंग (जो सीधे ANDROID_ABI के हिसाब से ढूंढेगा)
find_path(NCNN_INCLUDE_DIR 
    NAMES ncnn/net.h
    PATHS 
        "${PROJECT_ROOT}/ncnn"
        "${CMAKE_SOURCE_DIR}/ncnn"
    PATH_SUFFIXES 
        "${ANDROID_ABI}/include"
        "arm64-v8a/include"
        "armeabi-v7a/include"
        "include"
)

# 2. लाइब्रेरी (libncnn.a) के लिए स्मार्ट हंटिंग
find_library(NCNN_LIBRARY 
    NAMES ncnn
    PATHS 
        "${PROJECT_ROOT}/ncnn"
        "${CMAKE_SOURCE_DIR}/ncnn"
    PATH_SUFFIXES 
        "${ANDROID_ABI}/lib"
        "arm64-v8a/lib"
        "armeabi-v7a/lib"
        "lib"
)

# सुरक्षा जांच: अगर कुत्ता खाली हाथ लौटा, तो साफ़-साफ़ एरर बताएगा
if(NOT NCNN_INCLUDE_DIR)
    message(FATAL_ERROR "भाई! ncnn/net.h नहीं मिली। पाथ चेक कर ले।")
endif()

if(NOT NCNN_LIBRARY)
    message(FATAL_ERROR "भाई! libncnn.a नहीं मिली। पाथ चेक कर ले।")
endif()

message(STATUS "सफलता! NCNN Include मिला: ${NCNN_INCLUDE_DIR}")
message(STATUS "सफलता! NCNN Library मिली: ${NCNN_LIBRARY}")

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
