cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")
set(CMAKE_SYSTEM_VERSION 26)

# अगर Gradle से PROJECT_ROOT नहीं मिला, तो गिटहब एक्शन का पक्का रूट पाथ ले लो
if(NOT PROJECT_ROOT)
    set(PROJECT_ROOT "/home/runner/work/My-project/My-project")
endif()

message(STATUS "PROJECT_ROOT is: ${PROJECT_ROOT}")
message(STATUS "Current ABI: ${ANDROID_ABI}")

# NCNN का 100% सटीक पाथ
set(NCNN_INCLUDE_DIR "${PROJECT_ROOT}/ncnn/${ANDROID_ABI}/include")
set(NCNN_LIB_DIR "${PROJECT_ROOT}/ncnn/${ANDROID_ABI}/lib")

message(STATUS "NCNN Include Path: ${NCNN_INCLUDE_DIR}")
message(STATUS "NCNN Lib Path: ${NCNN_LIB_DIR}")

include_directories(${NCNN_INCLUDE_DIR})

# OpenMP को NDK टूलचेन के साथ एक्टिवेट करें
find_package(OpenMP REQUIRED)

add_library(ncnn STATIC IMPORTED)
set_target_properties(ncnn PROPERTIES IMPORTED_LOCATION "${NCNN_LIB_DIR}/libncnn.a")

add_library(
    true-singularity-core
    SHARED
    true-singularity-core.cpp
)

find_library(log-lib log)
find_library(android-lib android)
find_library(jnigraphics-lib jnigraphics)

# कंपाइलर फ्लैग्स में OpenMP और C++ एक्सेप्शन्स/थ्रेड्स पक्के करें
target_compile_options(true-singularity-core PRIVATE -fopenmp -O3)
target_link_options(true-singularity-core PRIVATE -fopenmp)

target_link_libraries(
    true-singularity-core
    ncnn
    OpenMP::OpenMP_CXX
    ${jnigraphics-lib}
    ${android-lib}
    ${log-lib}
)
