Cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")

# अब यह सीधा Gradle से मिले ${PROJECT_ROOT} का इस्तेमाल करेगा (कोई पाथ भटकने का चांस नहीं)
include_directories(${PROJECT_ROOT}/ncnn/include)

add_library(ncnn STATIC IMPORTED)
set_target_properties(ncnn PROPERTIES IMPORTED_LOCATION
    ${PROJECT_ROOT}/ncnn/${ANDROID_ABI}/lib/libncnn.a)

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

