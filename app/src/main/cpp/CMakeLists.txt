cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")

# अब यह चुपचाप app/src/main/cpp/ncnn से उठा लेगा क्योंकि हमने एक्शन में वहीं कॉपी करवा दिया है
include_directories(ncnn/include)

add_library(ncnn STATIC IMPORTED)
set_target_properties(ncnn PROPERTIES IMPORTED_LOCATION
    ${CMAKE_CURRENT_SOURCE_DIR}/ncnn/lib/${ANDROID_ABI}/libncnn.a)

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
