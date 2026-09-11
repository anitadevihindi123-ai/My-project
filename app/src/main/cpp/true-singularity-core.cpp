#pragma clang diagnostic ignored "-Wunguarded-availability"
#include <jni.h>
#include <string>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <cstdio>
#include <dlfcn.h>
#include <android/log.h>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <vulkan/vulkan_android.h>
#include <android/hardware_buffer.h>
#include <android/hardware_buffer_jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#define LOG_TAG "NativeLoader"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define VK_CHECK(call) \
    do { \
        VkResult result_ = call; \
        if (result_ != VK_SUCCESS) { \
            LOGE("Vulkan Critical Error: %s returned VkResult %d at line %d", #call, result_, __LINE__); \
            throw std::runtime_error("Vulkan API failure in " + std::string(#call)); \
        } \
    } while (0)
typedef AHardwareBuffer* (*PFN_AHardwareBuffer_fromHardwareBuffer)(JNIEnv* env, jobject hardwareBuffer);
typedef void (*PFN_AHardwareBuffer_release)(AHardwareBuffer* buffer);
typedef struct native_handle {
    int version;
    int numFds;
    int numInts;
    int data[0];
} native_handle_t;
class AndroidNativeLoader {
private:
    void* handle_;
    bool is_initialized_;
    std::mutex mutex_;
    PFN_AHardwareBuffer_fromHardwareBuffer fn_AHardwareBuffer_fromHardwareBuffer_;
    PFN_AHardwareBuffer_release fn_AHardwareBuffer_release_;

    AndroidNativeLoader() : handle_(nullptr), is_initialized_(false),
                            fn_AHardwareBuffer_fromHardwareBuffer_(nullptr),
                            fn_AHardwareBuffer_release_(nullptr) {}

    ~AndroidNativeLoader() {
        if (handle_) { dlclose(handle_); handle_ = nullptr; }
    }

public:
    static AndroidNativeLoader& getInstance() {
        static AndroidNativeLoader instance;
        return instance;
    }

    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_initialized_) return true;
        handle_ = dlopen("libandroid.so", RTLD_LAZY | RTLD_LOCAL);
        if (!handle_) return false;
        fn_AHardwareBuffer_fromHardwareBuffer_ = reinterpret_cast<PFN_AHardwareBuffer_fromHardwareBuffer>(dlsym(handle_, "AHardwareBuffer_fromHardwareBuffer"));
        fn_AHardwareBuffer_release_ = reinterpret_cast<PFN_AHardwareBuffer_release>(dlsym(handle_, "AHardwareBuffer_release"));
        if (!fn_AHardwareBuffer_fromHardwareBuffer_ || !fn_AHardwareBuffer_release_) {
            dlclose(handle_); handle_ = nullptr; return false;
        }
        is_initialized_ = true;
        return true;
    }

    AHardwareBuffer* createFromJava(JNIEnv* env, jobject hardwareBuffer) {
        if (!is_initialized_ || !fn_AHardwareBuffer_fromHardwareBuffer_) {
            throw std::runtime_error("AndroidNativeLoader not initialized or symbol missing.");
        }
        return fn_AHardwareBuffer_fromHardwareBuffer_(env, hardwareBuffer);
    }

    void releaseBuffer(AHardwareBuffer* buffer) {
        if (is_initialized_ && fn_AHardwareBuffer_release_ && buffer) {
            fn_AHardwareBuffer_release_(buffer);
        }
    }

    AndroidNativeLoader(const AndroidNativeLoader&) = delete;
    AndroidNativeLoader& operator=(const AndroidNativeLoader&) = delete;
};

extern "C" {
    const native_handle_t* AHardwareBuffer_getNativeHandle(const AHardwareBuffer* buffer);
}

#define MAX_FRAMES_IN_FLIGHT 2

struct FinalCachedImage {
    VkImage vkImage = VK_NULL_HANDLE;
    VkDeviceMemory vkMemory = VK_NULL_HANDLE;
    VkImageView vkImageView = VK_NULL_HANDLE;
    int kernelDmaBufFd = -1;
    uint32_t width = 0;
    uint32_t height = 0;
    bool isAllocated = false;
};

struct FinalFrameContext {
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    uint64_t timelineTargetValue = 0;
    VkImageView frameOutputView = VK_NULL_HANDLE;
};

struct FinalConstants {
    float zoomFactor;
    float thermalLoad;
    float gyroShiftX;
    float gyroShiftY;
    int width;
    int height;
    float _pad0;
    float _pad1;
    float viewMatrix[16];
};

class PureMetalEngine {
public:
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0;

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline computePipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkSampler defaultSampler = VK_NULL_HANDLE;

    FinalFrameContext frames[MAX_FRAMES_IN_FLIGHT];
    uint32_t currentFrameIndex = 0;
    VkSemaphore timelineSemaphore = VK_NULL_HANDLE;
    std::atomic<uint64_t> globalTimelineCounter{0};

    std::atomic<float> thermalLoad{0.1f};
    std::atomic<float> gyroShiftX{0.0f};
    std::atomic<float> gyroShiftY{0.0f};
        float viewMatrix[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    std::unordered_map<AHardwareBuffer*, FinalCachedImage> ringBufferCache;
    std::mutex poolMutex;
    std::vector<VkImageView> recentImageViews;
  ANativeWindow* nativeWindow = nullptr;
VkSurfaceKHR surface = VK_NULL_HANDLE;
VkSwapchainKHR swapchain = VK_NULL_HANDLE;
std::vector<VkImage> swapchainImages;
std::vector<VkImageView> swapchainImageViews;
uint32_t swapchainImageCount = 0;
    mutable std::shared_mutex surfaceMutex;
    std::atomic<bool> isSurfaceActive{false};

    bool initialized = false;
    PFN_vkWaitSemaphores pfnVkWaitSemaphores = nullptr;
    std::atomic<float> cachedTemperature{45.0f};
std::atomic<bool> thermalRunning{true};
int thermalFd = -1;
std::thread thermalThread;
    VkImageView GetOrCreateImageViewFromAHB(AHardwareBuffer* ahb) {
        auto it = ringBufferCache.find(ahb);
        if (it != ringBufferCache.end() && it->second.vkImageView != VK_NULL_HANDLE) {
            return it->second.vkImageView;
        }

        AHardwareBuffer_Desc desc;
        AHardwareBuffer_describe(ahb, &desc);

        FinalCachedImage newImg = {};
        AHardwareBuffer_acquire(ahb);

        VkExternalMemoryImageCreateInfo extInfo = {};
        extInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
        extInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID;

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.pNext = &extInfo;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.extent.width = desc.width;
        imageInfo.extent.height = desc.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_GENERAL;

        VK_CHECK(vkCreateImage(device, &imageInfo, nullptr, &newImg.vkImage));
           auto fpGetProps = reinterpret_cast<PFN_vkGetAndroidHardwareBufferPropertiesANDROID>(
                vkGetDeviceProcAddr(device, "vkGetAndroidHardwareBufferPropertiesANDROID")
            );

            if (fpGetProps) {
                VkAndroidHardwareBufferPropertiesANDROID ahbProps = {};
                ahbProps.sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID;

                if (fpGetProps(device, ahb, &ahbProps) == VK_SUCCESS) {
                    VkImportAndroidHardwareBufferInfoANDROID importHb = {};
                    importHb.sType = VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID;
                    importHb.buffer = ahb;

                    VkMemoryDedicatedAllocateInfo dedicatedAllocInfo = {};
                    dedicatedAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
                    dedicatedAllocInfo.pNext = &importHb;
                    dedicatedAllocInfo.image = newImg.vkImage;

                    VkPhysicalDeviceMemoryProperties memProps;
                    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

                    uint32_t memTypeIdx = 0;
                    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
                        if ((ahbProps.memoryTypeBits & (1 << i))) {
                            memTypeIdx = i;
                            break;
                        }
                    }

                    VkMemoryAllocateInfo allocInfo = {};
                    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                    allocInfo.pNext = &dedicatedAllocInfo;
                    allocInfo.allocationSize = ahbProps.allocationSize;
                    allocInfo.memoryTypeIndex = memTypeIdx;

                    VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &newImg.vkMemory));
VK_CHECK(vkBindImageMemory(device, newImg.vkImage, newImg.vkMemory, 0));

                        VkImageViewCreateInfo viewInfo = {};
                        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                        viewInfo.image = newImg.vkImage;
                        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
                        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        viewInfo.subresourceRange.levelCount = 1;
                        viewInfo.subresourceRange.layerCount = 1;

                        vkCreateImageView(device, &viewInfo, nullptr, &newImg.vkImageView);
                        newImg.width = desc.width;
                        newImg.height = desc.height;
                        newImg.isAllocated = true;

                        std::lock_guard<std::mutex> lock(poolMutex);
                        ringBufferCache[ahb] = newImg;
                        return newImg.vkImageView;
                    }
                }
            }
        }
        return VK_NULL_HANDLE;
    }

    ~PureMetalEngine() {
       thermalRunning = false;
if (thermalThread.joinable()) {
    thermalThread.join();
}
if (thermalFd >= 0) {
    close(thermalFd);
    thermalFd = -1;
}
 if (device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device);
            std::lock_guard<std::mutex> lock(poolMutex);
            for (auto& pair : ringBufferCache) {
                if (pair.second.vkImageView != VK_NULL_HANDLE) vkDestroyImageView(device, pair.second.vkImageView, nullptr);
                if (pair.second.vkImage != VK_NULL_HANDLE) vkDestroyImage(device, pair.second.vkImage, nullptr);
                if (pair.second.vkMemory != VK_NULL_HANDLE) vkFreeMemory(device, pair.second.vkMemory, nullptr);
                if (pair.first) AHardwareBuffer_release(pair.first);
            }
            ringBufferCache.clear();
            if (timelineSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(device, timelineSemaphore, nullptr);
            if (descriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            if (computePipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, computePipeline, nullptr);
            if (pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            if (descriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
            if (shaderModule != VK_NULL_HANDLE) vkDestroyShaderModule(device, shaderModule, nullptr);
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                if (frames[i].commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(device, frames[i].commandPool, nullptr);
            }
            vkDestroyDevice(device, nullptr);
        }
        if (instance != VK_NULL_HANDLE) {
            vkDestroyInstance(instance, nullptr);
        }
    }

        void initThermalMonitor() {
        // **[रॉ इंजीनियरिंग डायनेमिक पाथ स्कैनिंग]**
        const char* possiblePaths[] = {
            "/sys/class/thermal/thermal_zone0/temp",
            "/sys/class/thermal/thermal_zone1/temp",
            "/sys/class/thermal/thermal_zone2/temp",
            "/sys/devices/virtual/thermal/thermal_zone0/temp"
        };

        for (const char* path : possiblePaths) {
            thermalFd = open(path, O_RDONLY | O_NONBLOCK);
            if (thermalFd >= 0) {
                break; // जैसे ही सही थर्मल जोन फाइल मिल जाएगी, लूप ब्रेक हो जाएगा
            }
        }

        thermalThread = std::thread([this]() {
            char buffer[64];
            struct pollfd pfd;
            pfd.fd = thermalFd;
            pfd.events = POLLPRI | POLLERR;

            while (thermalRunning) {
                if (thermalFd >= 0) {
                    int ret = poll(&pfd, 1, 2000);
                    if (ret >= 0) {
                        lseek(thermalFd, 0, SEEK_SET);
                        int bytes = read(thermalFd, buffer, sizeof(buffer) - 1);
                        if (bytes > 0) {
                            buffer[bytes] = '\0';
                            try {
                                float temp = std::stof(buffer) / 1000.0f;
                                cachedTemperature.store(temp, std::memory_order_relaxed);
                            } catch (...) {}
                        }
                    }
                }
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });
    }

    uint32_t readKernelThermalRegister() {
        float temp = cachedTemperature.load(std::memory_order_relaxed);
        return static_cast<uint32_t>(temp * 1000.0f);
    }

    void ignite(AAssetManager* assetManager) {
    if (initialized) return;
    initThermalMonitor();

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "PureMetalEngine";
    appInfo.apiVersion = VK_API_VERSION_1_1;

    const char* instExtensions[] = {
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
    };

    VkInstanceCreateInfo instInfo = {};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = 2;
    instInfo.ppEnabledExtensionNames = instExtensions;
    instInfo.enabledLayerCount = 0;

    VK_CHECK(vkCreateInstance(&instInfo, nullptr, &instance));

    uint32_t devCount = 0;
    vkEnumeratePhysicalDevices(instance, &devCount, nullptr);
    if (devCount == 0) return;
    std::vector<VkPhysicalDevice> devs(devCount);
    vkEnumeratePhysicalDevices(instance, &devCount, devs.data());
    physicalDevice = devs[0];

    uint32_t qCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qCount, nullptr);
    std::vector<VkQueueFamilyProperties> qFamilies(qCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qCount, qFamilies.data());

    uint32_t idx = 0;
    for (const auto& qf : qFamilies) {
        if (qf.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueFamilyIndex = idx;
            break;
        }
        idx++;
    }

    float priority = 1.0f;
    VkDeviceQueueCreateInfo qInfo = {};
    qInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qInfo.queueFamilyIndex = queueFamilyIndex;
    qInfo.queueCount = 1;
    qInfo.pQueuePriorities = &priority;

    const char* devExtensions[] = {
        "VK_ANDROID_external_memory_android_hardware_buffer",
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
        VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
        VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME
    };

    VkDeviceCreateInfo devInfo = {};
    devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devInfo.queueCreateInfoCount = 1;
    devInfo.pQueueCreateInfos = &qInfo;
    devInfo.enabledExtensionCount = 4;
    devInfo.ppEnabledExtensionNames = devExtensions;

    VK_CHECK(vkCreateDevice(physicalDevice, &devInfo, nullptr, &device));
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &computeQueue);

    if (assetManager) {
        AAsset* asset = AAssetManager_open(assetManager, "singularity_compute.spv", AASSET_MODE_STREAMING);
        if (asset) {
            size_t size = static_cast<size_t>(AAsset_getLength(asset));
            std::vector<char> shaderCode(size);
            AAsset_read(asset, shaderCode.data(), size);
            AAsset_close(asset);

            VkShaderModuleCreateInfo shaderInfo = {};
            shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderInfo.codeSize = shaderCode.size();
            shaderInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
            VK_CHECK(vkCreateShaderModule(device, &shaderInfo, nullptr, &shaderModule));
        }
    }

    VkDescriptorSetLayoutBinding bindings[3] = {};
    for (int i = 0; i < 3; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings = bindings;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout));

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(FinalConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    if (shaderModule != VK_NULL_HANDLE) {
        VkComputePipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        pipelineInfo.stage.module = shaderModule;
        pipelineInfo.stage.pName = "main";
        pipelineInfo.layout = pipelineLayout;
        VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline));
    }

    VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 * MAX_FRAMES_IN_FLIGHT };
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));

    VkSemaphoreTypeCreateInfo timelineInfo = {};
    timelineInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timelineInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineInfo.initialValue = 0;

    VkSemaphoreCreateInfo semInfo = {};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semInfo.pNext = &timelineInfo;
    VK_CHECK(vkCreateSemaphore(device, &semInfo, nullptr, &timelineSemaphore));

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &frames[i].commandPool));

        VkCommandBufferAllocateInfo cmdAllocInfo = {};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.commandPool = frames[i].commandPool;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount = 1;
        VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].commandBuffer));

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &frames[i].descriptorSet));

        frames[i].timelineTargetValue = 0;
        frames[i].frameOutputView = VK_NULL_HANDLE;
    }

    pfnVkWaitSemaphores = reinterpret_cast<PFN_vkWaitSemaphores>(
        vkGetDeviceProcAddr(device, "vkWaitSemaphores")
    );

    initialized = true;
}


void initWindow(ANativeWindow* window) {
    std::unique_lock<std::shared_mutex> lock(surfaceMutex);
    nativeWindow = window;
    if (!instance || !physicalDevice || !device) return;

    VkAndroidSurfaceCreateInfoKHR surfInfo = {};
    surfInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfInfo.window = nativeWindow;
    if (vkCreateAndroidSurfaceKHR(instance, &surfInfo, nullptr, &surface) != VK_SUCCESS) return;

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);

    VkSwapchainCreateInfoKHR swapInfo = {};
    swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapInfo.surface = surface;
    swapInfo.minImageCount = 2;
    swapInfo.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    swapInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapInfo.imageExtent = caps.currentExtent;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapInfo.preTransform = caps.currentTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

    if (vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapchain) != VK_SUCCESS) return;
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);
    swapchainImages.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data());

    swapchainImageViews.resize(swapchainImageCount);
    for (size_t i = 0; i < swapchainImageCount; i++) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[i]);
    }

    // रेंडर थ्रेड के लिए स्टेट को एक्टिवेट करें
    isSurfaceActive.store(true, std::memory_order_release);
}

void destroyWindow() {
    // नए फ्रेम्स की एंट्री तुरंत ब्लॉक करें ताकि रेस कंडीशन का खतरा शून्य हो जाए
    isSurfaceActive.store(false, std::memory_order_release);

    std::unique_lock<std::shared_mutex> lock(surfaceMutex);
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
        for (auto v : swapchainImageViews) {
            if (v != VK_NULL_HANDLE) vkDestroyImageView(device, v, nullptr);
        }
        swapchainImageViews.clear();
        if (swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            swapchain = VK_NULL_HANDLE;
        }
        if (surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(instance, surface, nullptr);
            surface = VK_NULL_HANDLE;
        }
    }
    if (nativeWindow) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = nullptr;
    }
}
};
static PureMetalEngine* g_finalEngine = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_com_my_newproject_truesingularityclass_nativeExecuteZeroCopyPipeline(
        JNIEnv *env, jobject thiz, jobject hardwareBufferObj, jfloat zoomFactor, jlong frameIndex) {

    if (!hardwareBufferObj || !g_finalEngine || !g_finalEngine->initialized) return;
    // रॉ इंजीनियरिंग सरफेस लॉक
    std::shared_lock<std::shared_mutex> lock(g_finalEngine->surfaceMutex);
    if (!g_finalEngine->isSurfaceActive.load(std::memory_order_acquire)) {
        return; 
    }
    uint32_t rawTemp = g_finalEngine->readKernelThermalRegister();
    float thermalNorm = static_cast<float>(rawTemp) / 100000.0f;
    g_finalEngine->thermalLoad.store(thermalNorm);

    if (thermalNorm > 75.0f && (frameIndex % 2 != 0)) {
        return; 
    }

    float gX = g_finalEngine->gyroShiftX.load();
    float gY = g_finalEngine->gyroShiftY.load();

    static auto fromHb = reinterpret_cast<struct AHardwareBuffer*(*)(JNIEnv*, jobject)>(
        dlsym(dlopen("libandroid.so", RTLD_LAZY), "AHardwareBuffer_fromHardwareBuffer")
    );
    AHardwareBuffer* hb = fromHb ? fromHb(env, hardwareBufferObj) : nullptr;
    if (!hb) return;

    AHardwareBuffer_Desc desc;
    AHardwareBuffer_describe(hb, &desc);

    uint32_t curFrameIdx = g_finalEngine->currentFrameIndex;
    FinalFrameContext& frame = g_finalEngine->frames[curFrameIdx];
    g_finalEngine->currentFrameIndex = (curFrameIdx + 1) % MAX_FRAMES_IN_FLIGHT;

        if (frame.timelineTargetValue > 0 && g_finalEngine->pfnVkWaitSemaphores) {
        VkSemaphoreWaitInfo waitInfo = {};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &g_finalEngine->timelineSemaphore;
        waitInfo.pValues = &frame.timelineTargetValue;
        g_finalEngine->pfnVkWaitSemaphores(g_finalEngine->device, &waitInfo, UINT64_MAX);
    }


    FinalCachedImage cachedImg;
    bool needsAllocation = false;
    {
        std::lock_guard<std::mutex> lock(g_finalEngine->poolMutex);
        auto it = g_finalEngine->ringBufferCache.find(hb);
        if (it != g_finalEngine->ringBufferCache.end()) {
            cachedImg = it->second;
        } else {
            needsAllocation = true;
        }
    }
                const size_t MAX_CACHE_SIZE = 16;
    {
        std::lock_guard<std::mutex> lock(g_finalEngine->poolMutex);
        if (g_finalEngine->ringBufferCache.size() >= MAX_CACHE_SIZE) {
            auto oldestIt = g_finalEngine->ringBufferCache.begin();
            if (oldestIt != g_finalEngine->ringBufferCache.end()) {
                // यह लाइन पुरानी लाइन के ऊपर लगानी है:
                vkDeviceWaitIdle(g_finalEngine->device);

                if (oldestIt->second.vkImageView != VK_NULL_HANDLE) vkDestroyImageView(g_finalEngine->device, oldestIt->second.vkImageView, nullptr);
                if (oldestIt->second.vkImage != VK_NULL_HANDLE) vkDestroyImage(g_finalEngine->device, oldestIt->second.vkImage, nullptr);
                if (oldestIt->second.vkMemory != VK_NULL_HANDLE) vkFreeMemory(g_finalEngine->device, oldestIt->second.vkMemory, nullptr);
                if (oldestIt->first) AHardwareBuffer_release(oldestIt->first);
                g_finalEngine->ringBufferCache.erase(oldestIt);
            }
        }
    }


    if (needsAllocation) {
        FinalCachedImage newImg = {};
        AHardwareBuffer_acquire(hb);
        const native_handle_t* nativeHandle = AHardwareBuffer_getNativeHandle(hb);
        if (nativeHandle && nativeHandle->numFds > 0) {
            newImg.kernelDmaBufFd = nativeHandle->data[0];
        }

        VkExternalMemoryImageCreateInfo extInfo = {};
        extInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
        extInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID;

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.pNext = &extInfo;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.extent.width = desc.width;
        imageInfo.extent.height = desc.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 5; // [HARDCORE FIX]: Adjusted arrayLayers to match GLSL image2DArray 5-layer requirement
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_GENERAL;

      VK_CHECK(vkCreateImage(g_finalEngine->device, &imageInfo, nullptr, &newImg.vkImage));  
        auto fpGetProps = reinterpret_cast<PFN_vkGetAndroidHardwareBufferPropertiesANDROID>(
                vkGetDeviceProcAddr(g_finalEngine->device, "vkGetAndroidHardwareBufferPropertiesANDROID")
            );

            if (fpGetProps) {
                VkAndroidHardwareBufferPropertiesANDROID ahbProps = {};
                ahbProps.sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID;

                if (fpGetProps(g_finalEngine->device, hb, &ahbProps) == VK_SUCCESS) {
                    VkImportAndroidHardwareBufferInfoANDROID importHb = {};
                    importHb.sType = VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID;
                    importHb.buffer = hb;

                    VkMemoryDedicatedAllocateInfo dedicatedAllocInfo = {};
                    dedicatedAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
                    dedicatedAllocInfo.pNext = &importHb;
                    dedicatedAllocInfo.image = newImg.vkImage;

                    VkPhysicalDeviceMemoryProperties memProps;
                    vkGetPhysicalDeviceMemoryProperties(g_finalEngine->physicalDevice, &memProps);

                    uint32_t memTypeIdx = 0;
                    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
                        if ((ahbProps.memoryTypeBits & (1 << i))) {
                            if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                                memTypeIdx = i;
                                break;
                            }
                            memTypeIdx = i;
                        }
                    }

                    VkMemoryAllocateInfo allocInfo = {};
                    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                    allocInfo.pNext = &dedicatedAllocInfo;
                    allocInfo.allocationSize = ahbProps.allocationSize;
                    allocInfo.memoryTypeIndex = memTypeIdx;

                    VK_CHECK(vkAllocateMemory(g_finalEngine->device, &allocInfo, nullptr, &newImg.vkMemory));
VK_CHECK(vkBindImageMemory(g_finalEngine->device, newImg.vkImage, newImg.vkMemory, 0));

                        VkImageViewCreateInfo viewInfo = {};
                        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                        viewInfo.image = newImg.vkImage;
                        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; // [HARDCORE FIX]: Upgraded viewType to 2D Array for safe multi-layer indexing
                        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
                        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        viewInfo.subresourceRange.levelCount = 1;
                        viewInfo.subresourceRange.baseArrayLayer = 0;
                        viewInfo.subresourceRange.layerCount = 5; // [HARDCORE FIX]: Expose all 5 slices to eliminate out-of-bound shader evaluation faults

                        vkCreateImageView(g_finalEngine->device, &viewInfo, nullptr, &newImg.vkImageView);
                        newImg.width = desc.width;
                        newImg.height = desc.height;
                        newImg.isAllocated = true;

                        std::lock_guard<std::mutex> lock(g_finalEngine->poolMutex);
                        g_finalEngine->ringBufferCache[hb] = newImg;
                        cachedImg = newImg;
                    }
                }
            }
        }
    }

    if (cachedImg.vkImageView != VK_NULL_HANDLE) {
        uint32_t prevFrameIdx = (curFrameIdx == 0) ? (MAX_FRAMES_IN_FLIGHT - 1) : (curFrameIdx - 1);
        VkImageView temporalView = g_finalEngine->frames[prevFrameIdx].frameOutputView;
        if (temporalView == VK_NULL_HANDLE) {
            temporalView = cachedImg.vkImageView;
        }

  VkDescriptorImageInfo imgDesc[3] = {};
    imgDesc[0].imageView = cachedImg.vkImageView;
    imgDesc[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    imgDesc[1].imageView = cachedImg.vkImageView;
    imgDesc[1].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    g_finalEngine->recentImageViews.push_back(cachedImg.vkImageView);
    if (g_finalEngine->recentImageViews.size() > 5) {
        g_finalEngine->recentImageViews.erase(g_finalEngine->recentImageViews.begin());
    }
    imgDesc[2].imageView = g_finalEngine->recentImageViews.back();
    imgDesc[2].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet writes[3] = {};
    for (int i = 0; i < 3; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = frame.descriptorSet;
        writes[i].dstBinding = i;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[i].descriptorCount = 1;
        writes[i].pImageInfo = &imgDesc[i];
    }


        vkUpdateDescriptorSets(g_finalEngine->device, 3, writes, 0, nullptr);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(frame.commandBuffer, &beginInfo);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = cachedImg.vkImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 5; // [HARDCORE FIX]: Match barrier subresource range to 5 layers to prevent validation layout hazards

        vkCmdPipelineBarrier(frame.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, g_finalEngine->computePipeline);
        vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, g_finalEngine->pipelineLayout, 0, 1, &frame.descriptorSet, 0, nullptr);

        FinalConstants pc = {};
        pc.zoomFactor = zoomFactor;
        pc.thermalLoad = thermalNorm;
        pc.gyroShiftX = gX;
        pc.gyroShiftY = gY;
        pc.width = static_cast<int>(desc.width);
        pc.height = static_cast<int>(desc.height);
        pc._pad0 = 0.0f;
        pc._pad1 = 0.0f;
        
        for (int i = 0; i < 16; ++i) {
    pc.viewMatrix[i] = g_finalEngine->viewMatrix[i];
}

        vkCmdPushConstants(frame.commandBuffer, g_finalEngine->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);
        vkCmdDispatch(frame.commandBuffer, (desc.width + 15) / 16, (desc.height + 15) / 16, 1);

        vkEndCommandBuffer(frame.commandBuffer);

        uint64_t sigVal = ++g_finalEngine->globalTimelineCounter;
        frame.timelineTargetValue = sigVal;

        VkTimelineSemaphoreSubmitInfo timeSub = {};
        timeSub.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timeSub.signalSemaphoreValueCount = 1;
        timeSub.pSignalSemaphoreValues = &sigVal;

        VkSubmitInfo submit = {};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.pNext = &timeSub;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &frame.commandBuffer;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &g_finalEngine->timelineSemaphore;

        vkQueueSubmit(g_finalEngine->computeQueue, 1, &submit, VK_NULL_HANDLE);

        frame.frameOutputView = cachedImg.vkImageView;
    }
}
extern "C" JNIEXPORT void JNICALL
Java_com_my_newproject_truesingularityclass_nativeInitMasterEngine(
        JNIEnv *env, jobject thiz, jlong seed, jint targetWidth, jint targetHeight) {
    if (!g_finalEngine) {
        g_finalEngine = new PureMetalEngine();
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_my_newproject_truesingularityclass_nativeInitAssetManager(
        JNIEnv *env, jobject thiz, jobject assetManagerObj) {
    if (g_finalEngine) {
        AAssetManager* assetManager = AAssetManager_fromJava(env, assetManagerObj);
        g_finalEngine->ignite(assetManager);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_my_newproject_truesingularityclass_nativeDestroyMasterEngine(
        JNIEnv *env, jobject thiz) {
    if (g_finalEngine) {
        delete g_finalEngine;
        g_finalEngine = nullptr;
    }
}
extern "C" JNIEXPORT void JNICALL
Java_com_my_newproject_truesingularityclass_nativeUpdateViewMatrix(
        JNIEnv *env, jobject thiz, jfloatArray matrixArray) {
    if (!g_finalEngine || !g_finalEngine->initialized) return;
    jfloat* elems = env->GetFloatArrayElements(matrixArray, nullptr);
    if (elems) {
        for (int i = 0; i < 16; ++i) {
            g_finalEngine->viewMatrix[i] = elems[i];
        }
        env->ReleaseFloatArrayElements(matrixArray, elems, JNI_ABORT);
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_my_newproject_truesingularityclass_nativeGetZoomShader(
        JNIEnv *env, jobject thiz, jfloat zoomFactor) {
    std::string shaderInfo = "Active Zoom Shader (Factor: " + std::to_string(zoomFactor) + ")";
    return env->NewStringUTF(shaderInfo.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_my_newproject_truesingularityclass_nativeExecuteMultiFrameRawStacking(
        JNIEnv *env, jobject thiz, jobjectArray hardwareBuffersArray) {
    if (!g_finalEngine || !g_finalEngine->initialized || !hardwareBuffersArray) return;

    jsize count = env->GetArrayLength(hardwareBuffersArray);
    if (count <= 0) return;

    static auto fromHb = reinterpret_cast<struct AHardwareBuffer*(*)(JNIEnv*, jobject)>(
        dlsym(dlopen("libandroid.so", RTLD_LAZY), "AHardwareBuffer_fromHardwareBuffer")
    );
    if (!fromHb) return;

    std::vector<AHardwareBuffer*> frameBuffers;
    uint32_t imgWidth = 0;
    uint32_t imgHeight = 0;

    for (jsize i = 0; i < count; ++i) {
        jobject hbObj = env->GetObjectArrayElement(hardwareBuffersArray, i);
        if (hbObj) {
            AHardwareBuffer* hb = fromHb(env, hbObj);
            if (hb) {
                frameBuffers.push_back(hb);
                // **[फिक्स 1]: यहाँ से इमेज की सही चौड़ाई और ऊँचाई निकाली जा रही है**
                if (imgWidth == 0) {
                    AHardwareBuffer_Desc desc;
                    AHardwareBuffer_describe(hb, &desc);
                    imgWidth = desc.width;
                    imgHeight = desc.height;
                }
            }
            env->DeleteLocalRef(hbObj);
        }
    }

    if (frameBuffers.empty()) return;

    uint32_t curFrameIdx = g_finalEngine->currentFrameIndex;
    FinalFrameContext& frame = g_finalEngine->frames[curFrameIdx];
    g_finalEngine->currentFrameIndex = (curFrameIdx + 1) % MAX_FRAMES_IN_FLIGHT;

    std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    imageInfos.resize(frameBuffers.size());

    for (size_t i = 0; i < frameBuffers.size(); ++i) {
        imageInfos[i].sampler = g_finalEngine->defaultSampler;
        imageInfos[i].imageView = g_finalEngine->GetOrCreateImageViewFromAHB(frameBuffers[i]); 
        // **[फिक्स 2]: स्टोरेज इमेज के लिए सही लेआउट**
        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = frame.descriptorSet;
        write.dstBinding = 0; 
        write.dstArrayElement = static_cast<uint32_t>(i);
        // **[फिक्स 3]: सही डिस्criptor टाइप**
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfos[i];
        writeDescriptorSets.push_back(write);
    }

    vkUpdateDescriptorSets(g_finalEngine->device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(frame.commandBuffer, &beginInfo);
    vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, g_finalEngine->computePipeline);
    vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, g_finalEngine->pipelineLayout, 0, 1, &frame.descriptorSet, 0, nullptr);
    
    vkCmdDispatch(frame.commandBuffer, (imgWidth + 15) / 16, (imgHeight + 15) / 16, 1);

    vkEndCommandBuffer(frame.commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame.commandBuffer;

    vkQueueSubmit(g_finalEngine->computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(g_finalEngine->computeQueue);

    for (auto* hb : frameBuffers) {
        if (hb) {
            AHardwareBuffer_release(hb);
        }
    }
}


extern "C" JNIEXPORT void JNICALL
Java_com_my_newproject_truesingularityclass_nativeApplyGyroStabilization(
        JNIEnv *env, jobject thiz, jfloat gyroX, jfloat gyroY, jfloat gyroZ) {
    if (!g_finalEngine || !g_finalEngine->initialized) return;
    g_finalEngine->gyroShiftX.store(gyroX);
    g_finalEngine->gyroShiftY.store(gyroY);
}
extern "C" JNIEXPORT jstring JNICALL
Java_com_my_newproject_truesingularityclass_nativeExecuteMasterOmniPipeline(
        JNIEnv *env, jobject thiz, jfloat zoomVal, jfloat temperatureVal) {
    if (!g_finalEngine || !g_finalEngine->initialized) {
        return env->NewStringUTF("Engine not initialized");
    }
    
    std::string result = "Master Omni Pipeline executed (Zoom: " + 
                         std::to_string(zoomVal) + ", Temp: " + std::to_string(temperatureVal) + ")";
    return env->NewStringUTF(result.c_str());
}
extern "C" JNIEXPORT void JNICALL
Java_com_my_newproject_truesingularityclass_nativeProcessDirectPixelBuffer(
        JNIEnv *env, jobject thiz, jobject hardwareBufferObj, jfloat zoomFactor, jlong frameIndex) {
    if (!hardwareBufferObj || !g_finalEngine || !g_finalEngine->initialized) return;

    static auto fromHb = reinterpret_cast<struct AHardwareBuffer*(*)(JNIEnv*, jobject)>(
        dlsym(dlopen("libandroid.so", RTLD_LAZY), "AHardwareBuffer_fromHardwareBuffer")
    );
    AHardwareBuffer* hb = fromHb ? fromHb(env, hardwareBufferObj) : nullptr;
    if (!hb) return;

    AHardwareBuffer_Desc desc;
    AHardwareBuffer_describe(hb, &desc);

    void *virtAddress = nullptr;
    int result = AHardwareBuffer_lock(
        hb, 
        AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, 
        -1, 
        nullptr, 
        &virtAddress
    );

    if (result != 0 || !virtAddress) {
        return; 
    }

    uint32_t *pixels = static_cast<uint32_t *>(virtAddress);
    for (uint32_t y = 0; y < desc.height; ++y) {
        uint32_t *row = pixels + (y * (desc.stride / 4)); 
        for (uint32_t x = 0; x < desc.width; ++x) {
            uint32_t pixel = row[x];
            row[x] = pixel; 
        }
    }

    AHardwareBuffer_unlock(hb, nullptr);
}


extern "C" JNIEXPORT void JNICALL
Java_com_my_newproject_truesingularityclass_nativeInitWindow(
        JNIEnv *env, jobject thiz, jobject surfaceObj) {
    if (!g_finalEngine) return;
    ANativeWindow* window = ANativeWindow_fromSurface(env, surfaceObj);
    g_finalEngine->initWindow(window);
}

extern "C" JNIEXPORT void JNICALL
Java_com_my_newproject_truesingularityclass_nativeDestroyWindow(
        JNIEnv *env, jobject thiz) {
    if (!g_finalEngine) return;
    g_finalEngine->destroyWindow();
}
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    AndroidNativeLoader::getInstance().initialize();
    return JNI_VERSION_1_6;
}
