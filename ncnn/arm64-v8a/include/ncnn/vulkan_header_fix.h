// Copyright 2020 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef NCNN_VULKAN_HEADER_FIX_H
#define NCNN_VULKAN_HEADER_FIX_H

#include <vulkan/vulkan_core.h>

// NDK r26 compatibility mapping for NCNN GPU features
#if defined(VK_VERSION_1_0)
typedef VkPhysicalDeviceCooperativeMatrixFeaturesNV VkPhysicalDeviceCooperativeMatrixFeaturesKHR;
typedef VkPhysicalDeviceCooperativeMatrixPropertiesNV VkPhysicalDeviceCooperativeMatrixPropertiesKHR;
typedef VkCooperativeMatrixPropertiesNV VkCooperativeMatrixPropertiesKHR;

#ifndef PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR
typedef PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR;
#endif
#endif

#endif // NCNN_VULKAN_HEADER_FIX_H
