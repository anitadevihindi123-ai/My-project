cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")

# सीधा और पक्का तरीका: CMAKE_SOURCE_DIR से 4 कदम पीछे जाकर My-project रूट सेट कर लो
get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}/../../../../" ABSOLUTE)

message(STATUS "===== ABSOLUTE PATH CHECK =====")
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "FIXED PROJECT_ROOT: ${PROJECT_ROOT}")
message(STATUS "ANDROID_ABI: ${ANDROID_ABI}")
message(STATUS "===============================")

# 1. हेडर फाइल ढूंढने का पक्का रास्ता
find_path(NCNN_INCLUDE_DIR 
    NAMES net.h
    PATHS 
        "${PROJECT_ROOT}/ncnn/${ANDROID_ABI}/include"
        "${PROJECT_ROOT}/ncnn/arm64-v8a/include"
        "${PROJECT_ROOT}/ncnn/armeabi-v7a/include"
    PATH_SUFFIXES ncnn
)

# 2. लाइब्रेरी ढूंढने का पक्का रास्ता
find_library(NCNN_LIBRARY 
    NAMES ncnn
    PATHS 
        "${PROJECT_ROOT}/ncnn/${ANDROID_ABI}/lib"
        "${PROJECT_ROOT}/ncnn/arm64-v8a/lib"
        "${PROJECT_ROOT}/ncnn/armeabi-v7a/lib"
)

# अगर फाइल न मिले तो साफ़-साफ़ एरर बताएगा
if(NOT NCNN_INCLUDE_DIR)
    message(FATAL_ERROR "त्रुटि: net.h इस पाथ पर नहीं मिली -> ${PROJECT_ROOT}/ncnn")
endif()

if(NOT NCNN_LIBRARY)
    message(FATAL_ERROR "त्रुटि: libncnn.a इस पाथ पर नहीं मिली -> ${PROJECT_ROOT}/ncnn")
endif()

message(STATUS "बल्ले-बल्ले! NCNN Include मिला: ${NCNN_INCLUDE_DIR}")
message(STATUS "बल्ले-बल्ले! NCNN Library मिली: ${NCNN_LIBRARY}")

# लाइब्रेरी और हेडर जोड़ो
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
