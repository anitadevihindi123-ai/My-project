cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")

# 1. प्रोजेक्ट के रूट फोल्डर से सीधा NCNN का पाथ सेट करना
include_directories(${CMAKE_SOURCE_DIR}/ncnn/include)

# 2. प्रोजेक्ट के रूट फोल्डर से सही आर्किटेक्चर की लाइब्रेरी उठाना
add_library(ncnn STATIC IMPORTED)
set_target_properties(ncnn PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/ncnn/lib/${ANDROID_ABI}/libncnn.a)

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
