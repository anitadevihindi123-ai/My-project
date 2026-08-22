// Copyright 2020 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef NCNN_VULKAN_HEADER_FIX_H
#define NCNN_VULKAN_HEADER_FIX_H

#include <vulkan/vulkan_core.h>

// --- NCNN to NDK r26 KHR/NV Compatibility Aliases ---
#if defined(VK_VERSION_1_0)

// 1. Cooperative Matrix Mappings
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

// 3. Shader & Additional Feature Mappings
typedef VkPhysicalDeviceShaderFloat16Int8Features VkPhysicalDeviceShaderFloatControls2FeaturesKHR;
typedef VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures VkPhysicalDeviceShaderSubgroupRotateFeaturesKHR;

#endif // VK_VERSION_1_0

#endif // NCNN_VULKAN_HEADER_FIX_H
