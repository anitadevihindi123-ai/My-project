cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")

get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}/../../../../" ABSOLUTE)

message(STATUS "=========================================")
message(STATUS "DEBUG CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "DEBUG PROJECT_ROOT:     ${PROJECT_ROOT}")
message(STATUS "DEBUG ANDROID_ABI:      ${ANDROID_ABI}")
message(STATUS "=========================================")

# 1. हेडर फाइल ढूंढने का पाथ
find_path(NCNN_INCLUDE_DIR 
    NAMES net.h
    PATHS 
        "${PROJECT_ROOT}/ncnn/${ANDROID_ABI}/include"
        "${PROJECT_ROOT}/ncnn/arm64-v8a/include"
        "${PROJECT_ROOT}/ncnn/armeabi-v7a/include"
    PATH_SUFFIXES ncnn
)

# 2. लाइब्रेरी ढूंढने का पाथ
find_library(NCNN_LIBRARY 
    NAMES ncnn
    PATHS 
        "${PROJECT_ROOT}/ncnn/${ANDROID_ABI}/lib"
        "${PROJECT_ROOT}/ncnn/arm64-v8a/lib"
        "${PROJECT_ROOT}/ncnn/armeabi-v7a/lib"
)

# अस्थायी रूप से FATAL_ERROR हटाकर चेतावनी (WARNING) कर दिया है 
# ताकि बिल्ड रुके ना और हमें पूरा सच पता चल जाए कि क्या मिला और क्या नहीं!
if(NOT NCNN_INCLUDE_DIR)
    message(WARNING "चेतावनी: net.h नहीं मिली! चेक किया गया पाथ: ${PROJECT_ROOT}/ncnn")
else()
    message(STATUS "सफलता! NCNN Include मिला: ${NCNN_INCLUDE_DIR}")
endif()

if(NOT NCNN_LIBRARY)
    message(WARNING "चेतावनी: libncnn.a नहीं मिली! चेक किया गया पाथ: ${PROJECT_ROOT}/ncnn")
else()
    message(STATUS "सफलता! NCNN Library मिली: ${NCNN_LIBRARY}")
endif()

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
