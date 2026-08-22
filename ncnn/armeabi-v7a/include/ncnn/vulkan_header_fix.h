// Copyright 2020 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef NCNN_VULKAN_HEADER_FIX_H
#define NCNN_VULKAN_HEADER_FIX_H

#include <vulkan/vulkan_core.h>

// --- NCNN to NDK r26 Full Compatibility & Stub Mappings ---
#if defined(VK_VERSION_1_0)

// 1. Cooperative Matrix & Vector Mappings
typedef VkPhysicalDeviceCooperativeMatrixFeaturesNV VkPhysicalDeviceCooperativeMatrixFeaturesKHR;
typedef VkPhysicalDeviceCooperativeMatrixPropertiesNV VkPhysicalDeviceCooperativeMatrixPropertiesKHR;
typedef VkCooperativeMatrixPropertiesNV VkCooperativeMatrixPropertiesKHR;
typedef VkComponentTypeNV VkComponentTypeKHR;
typedef VkScopeNV VkScopeKHR;

#ifndef PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR
typedef PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR;
#endif

// 2. Robustness2 Mappings
typedef VkPhysicalDeviceRobustness2FeaturesEXT VkPhysicalDeviceRobustness2FeaturesKHR;
typedef VkPhysicalDeviceRobustness2PropertiesEXT VkPhysicalDeviceRobustness2PropertiesKHR;

// 3. Shader Feature Mappings
typedef VkPhysicalDeviceShaderFloat16Int8Features VkPhysicalDeviceShaderFloatControls2FeaturesKHR;
typedef VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures VkPhysicalDeviceShaderSubgroupRotateFeaturesKHR;

// 4. Missing Stubs for Cooperative Matrix 2 & Vector (NDK r26 Fallbacks)
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

// Corrected Function Pointer Stubs for Cooperative Matrix / Vector
typedef void (VKAPI_PTR *PFN_vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV)(void);
typedef void (VKAPI_PTR *PFN_vkGetPhysicalDeviceCooperativeVectorPropertiesNV)(void);
typedef void (VKAPI_PTR *PFN_vkCmdConvertCooperativeVectorMatrixNV)(void);
typedef void (VKAPI_PTR *PFN_vkConvertCooperativeVectorMatrixNV)(void);
