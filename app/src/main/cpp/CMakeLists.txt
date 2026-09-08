cmake_minimum_required(VERSION 3.22.1)
project("true-singularity-core")
set(CMAKE_SYSTEM_VERSION 26)

# शुद्ध C++ और Vulkan इंजन लाइब्रेरी
add_library(
    true-singularity-core
    SHARED
    true-singularity-core.cpp
)

# एंड्रॉइड और Vulkan की जरूरी सिस्टम लाइब्रेरीज़ ढूंढना
find_library(log-lib log)
find_library(android-lib android)
find_library(jnigraphics-lib jnigraphics)
find_library(vulkan-lib vulkan)

# बिना किसी एआई के सीधे हार्डवेयर पर मैक्सिमम स्पीड (-O3 ऑप्टिमाइजेशन)
target_compile_options(true-singularity-core PRIVATE -O3 -std=c++17)

# सभी जरूरी नेटिव और Vulkan लाइब्रेरीज़ को आपस में जोड़ना
target_link_libraries(
    true-singularity-core
    vulkan
    ${jnigraphics-lib}
    ${android-lib}
    ${log-lib}
)

# APK का साइज और फालतू डेटा हटाने के लिए स्ट्रिप फ्लैग
set_target_properties(true-singularity-core PROPERTIES LINK_FLAGS "-Wl,--strip-all")
