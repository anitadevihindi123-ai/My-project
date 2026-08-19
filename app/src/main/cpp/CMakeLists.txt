cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")

# app/src/main/cpp से 4 कदम पीछे जाकर सीधे रूट वाले ncnn से हेडर उठाना
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../ncnn/include)

# 4 कदम पीछे जाकर रूट वाले ncnn से सही आर्किटेक्चर की लाइब्रेरी उठाना
add_library(ncnn STATIC IMPORTED)
set_target_properties(ncnn PROPERTIES IMPORTED_LOCATION
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../ncnn/lib/${ANDROID_ABI}/libncnn.a)

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
