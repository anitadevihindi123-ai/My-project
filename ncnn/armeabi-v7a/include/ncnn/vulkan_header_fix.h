// Copyright 2020 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef NCNN_VULKAN_HEADER_FIX_H
#define NCNN_VULKAN_HEADER_FIX_H

#include <vulkan/vulkan_core.h>

#if defined(VK_VERSION_1_0)

// 1. Cooperative Matrix & Vector Type Aliases
typedef VkPhysicalDeviceCooperativeMatrixFeaturesNV VkPhysicalDeviceCooperativeMatrixFeaturesKHR;
typedef VkPhysicalDeviceCooperativeMatrixPropertiesNV VkPhysicalDeviceCooperativeMatrixPropertiesKHR;
typedef VkCooperativeMatrixPropertiesNV VkCooperativeMatrixPropertiesKHR;
typedef VkComponentTypeNV VkComponentTypeKHR;
typedef VkScopeNV VkScopeKHR;

#ifndef PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR
typedef PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR;
#endif

// 2. Robustness2 Aliases
typedef VkPhysicalDeviceRobustness2FeaturesEXT VkPhysicalDeviceRobustness2FeaturesKHR;
typedef VkPhysicalDeviceRobustness2PropertiesEXT VkPhysicalDeviceRobustness2PropertiesKHR;

// 3. Shader Feature Aliases
typedef VkPhysicalDeviceShaderFloat16Int8Features VkPhysicalDeviceShaderFloatControls2FeaturesKHR;
typedef VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures VkPhysicalDeviceShaderSubgroupRotateFeaturesKHR;

// 4. Missing Struct Fallbacks for NDK r26
typedef struct VkPhysicalDeviceCooperativeMatrix2FeaturesNV {
    VkStructureType sType;
    void* pNext;
    VkBool32 cooperativeMatrix2;
} VkPhysicalDeviceCooperativeMatrix2FeaturesNV;

typedef struct VkPhysicalDeviceCooperativeVectorFeaturesNV {
    VkStructureType sType;
    void* pNext;
    VkBool32 cooperativeVector;
} VkPhysicalDeviceCooperativeVectorFeaturesNV;

typedef struct VkPhysicalDeviceShaderBfloat16FeaturesKHR {
    VkStructureType sType;
    void* pNext;
    VkBool32 shaderBfloat16;
} VkPhysicalDeviceShaderBfloat16FeaturesKHR;

typedef struct VkPhysicalDeviceShaderFloat8FeaturesEXT {
    VkStructureType sType;
    void* pNext;
    VkBool32 shaderFloat8;
} VkPhysicalDeviceShaderFloat8FeaturesEXT;

typedef struct VkPhysicalDeviceCooperativeMatrix2PropertiesNV {
    VkStructureType sType;
    void* pNext;
} VkPhysicalDeviceCooperativeMatrix2PropertiesNV;

typedef struct VkPhysicalDeviceCooperativeVectorPropertiesNV {
    VkStructureType sType;
    void* pNext;
} VkPhysicalDeviceCooperativeVectorPropertiesNV;

typedef struct VkCooperativeMatrixFlexibleDimensionsPropertiesNV {
    VkStructureType sType;
    void* pNext;
} VkCooperativeMatrixFlexibleDimensionsPropertiesNV;

typedef struct VkCooperativeVectorPropertiesNV {
    VkStructureType sType;
    void* pNext;
} VkCooperativeVectorPropertiesNV;

// 5. Correct Function Pointer Definitions (Avoiding type mismatches)
#ifndef PFN_vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV
typedef VkResult (VKAPI_PTR *PFN_vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV)(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkCooperativeMatrixFlexibleDimensionsPropertiesNV* pProperties);
#endif

#ifndef PFN_vkGetPhysicalDeviceCooperativeVectorPropertiesNV
typedef VkResult (VKAPI_PTR *PFN_vkGetPhysicalDeviceCooperativeVectorPropertiesNV)(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkCooperativeVectorPropertiesNV* pProperties);
#endif

#ifndef PFN_vkCmdConvertCooperativeVectorMatrixNV
typedef void (VKAPI_PTR *PFN_vkCmdConvertCooperativeVectorMatrixNV)(VkCommandBuffer commandBuffer);
#endif

#ifndef PFN_vkConvertCooperativeVectorMatrixNV
typedef VkResult (VKAPI_PTR *PFN_vkConvertCooperativeVectorMatrixNV)(VkDevice device);
#endif

#endif // VK_VERSION_1_0

#endif // NCNN_VULKAN_HEADER_FIX_H
