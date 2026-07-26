cmake_minimum_required(VERSION 3.10.2)
project("true-singularity-core")

add_library(
    true-singularity-core
    SHARED
    true-singularity-core.cpp
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(
    true-singularity-core
    ${log-lib}
    ${android-lib}
)
