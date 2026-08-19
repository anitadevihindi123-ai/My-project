cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")

# 1. NCNN का पाथ सेट करना (ताकि हेडर फाइलें मिल जाएं)
include_directories(${CMAKE_SOURCE_DIR}/../../../ncnn/include)

# 2. NCNN की प्री-बिल्ड लाइब्रेरी को जोड़ना (अलग-अलग आर्किटेक्चर के हिसाब से)
add_library(ncnn STATIC IMPORTED)
set_target_properties(ncnn PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/../../../ncnn/lib/${ANDROID_ABI}/libncnn.a)

add_library(
    true-singularity-core
    SHARED
    true-singularity-core.cpp
)

find_library(log-lib log)
find_library(android-lib android)

# 3. अपने प्रोजेक्ट के साथ ncnn को लिंक करना
target_link_libraries(
    true-singularity-core
    ncnn
    ${log-lib}
    ${android-lib}
)
