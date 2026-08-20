cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")

# --- यह रही वो डीबगिंग लाइन्स, इन्हें एकदम ऊपर रखा है ---
get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}/../../../" ABSOLUTE)
message(STATUS "===== DEBUG INFO START =====")
message(STATUS "PROJECT_ROOT is: ${PROJECT_ROOT}")
message(STATUS "CMAKE_SOURCE_DIR is: ${CMAKE_SOURCE_DIR}")
message(STATUS "Current ANDROID_ABI is: ${ANDROID_ABI}")
message(STATUS "===== DEBUG INFO END =======")

# 1. हेडर फाइल के लिए स्मार्ट हंटिंग
find_path(NCNN_INCLUDE_DIR 
    NAMES net.h
    PATHS 
        "${PROJECT_ROOT}/ncnn/${ANDROID_ABI}/include"
        "${PROJECT_ROOT}/ncnn/arm64-v8a/include"
        "${PROJECT_ROOT}/ncnn/armeabi-v7a/include"
    PATH_SUFFIXES ncnn
)

# 2. लाइब्रेरी के लिए स्मार्ट हंटिंग
find_library(NCNN_LIBRARY 
    NAMES ncnn
    PATHS 
        "${PROJECT_ROOT}/ncnn/${ANDROID_ABI}/lib"
        "${PROJECT_ROOT}/ncnn/arm64-v8a/lib"
        "${PROJECT_ROOT}/ncnn/armeabi-v7a/lib"
)

# सुरक्षा जांच: अगर कुत्ता खाली हाथ लौटा, तो साफ़-साफ़ एरर बताएगा
if(NOT NCNN_INCLUDE_DIR)
    message(FATAL_ERROR "भाई! net.h नहीं मिली। पाथ चेक कर ले।")
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
