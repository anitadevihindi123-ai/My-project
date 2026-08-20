cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")

# प्रोजेक्ट का रूट पाथ
get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}/../../../" ABSOLUTE)

# 1. हेडर फाइल के लिए सीधा और पक्का पाथ (जो डबल फोल्डर या नेस्टेड लेयर को भी पकड़ लेगा)
find_path(NCNN_INCLUDE_DIR 
    NAMES net.h
    PATHS 
        "${PROJECT_ROOT}/ncnn/${ANDROID_ABI}/include"
        "${PROJECT_ROOT}/ncnn/${ANDROID_ABI}/include/include"
        "${PROJECT_ROOT}/ncnn/arm64-v8a/include"
        "${PROJECT_ROOT}/ncnn/arm64-v8a/include/include"
        "${PROJECT_ROOT}/ncnn/armeabi-v7a/include"
        "${PROJECT_ROOT}/ncnn/armeabi-v7a/include/include"
    PATH_SUFFIXES ncnn
)

# 2. लाइब्रेरी के लिए सीधा और पक्का पाथ
find_library(NCNN_LIBRARY 
    NAMES ncnn
    PATHS 
        "${PROJECT_ROOT}/ncnn/${ANDROID_ABI}/lib"
        "${PROJECT_ROOT}/ncnn/arm64-v8a/lib"
        "${PROJECT_ROOT}/ncnn/armeabi-v7a/lib"
)

# सुरक्षा जांच: अगर फिर भी न मिले तो साफ़ पता चल जाए
if(NOT NCNN_INCLUDE_DIR)
    message(FATAL_ERROR "भाई! net.h इस पाथ पर नहीं मिल रही। पाथ चेक कर।")
endif()

if(NOT NCNN_LIBRARY)
    message(FATAL_ERROR "भाई! libncnn.a नहीं मिल रही। पाथ चेक कर।")
endif()

message(STATUS "सफलता! NCNN Include मिला: ${NCNN_INCLUDE_DIR}")
message(STATUS "सफलता! NCNN Library मिली: ${NCNN_LIBRARY}")

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
