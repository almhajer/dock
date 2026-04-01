#include "VulkanContext.h"

#include <GLFW/glfw3.h>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <set>

// تحميل الصور بدون مكتبات خارجية - نستخدم stb_image
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ─── ثوابت الطبقات والامتدادات ──────────────────────────────────

const std::vector<const char*> gfx::VulkanContext::VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> gfx::VulkanContext::DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

namespace gfx {

// ─── المُدمر ────────────────────────────────────────────────────

VulkanContext::~VulkanContext() {
    cleanup();
}

// ─── التهيئة الكاملة ───────────────────────────────────────────

void VulkanContext::init(GLFWwindow* window, uint32_t width, uint32_t height) {
    mWindowHandle = window;
    mWidth = width;
    mHeight = height;

    createInstance();
    setupDebugMessenger();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createFramebuffers();
    createCommandPool();
    createSpriteVertexBuffer();
    createSyncObjects();

    mInitialized = true;
    std::cout << "[Vulkan] تمت التهيئة بنجاح" << std::endl;
}

void VulkanContext::cleanup() {
    if (!mInitialized) return;
    mInitialized = false;
    mSwapchainDirty = false;

    vkDeviceWaitIdle(mDevice);

    cleanupSwapchain();
    cleanupSpriteDescriptors();

    if (mTextureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(mDevice, mTextureSampler, nullptr);
        mTextureSampler = VK_NULL_HANDLE;
    }
    if (mTextureImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(mDevice, mTextureImageView, nullptr);
        mTextureImageView = VK_NULL_HANDLE;
    }
    if (mTextureImage != VK_NULL_HANDLE) {
        vkDestroyImage(mDevice, mTextureImage, nullptr);
        mTextureImage = VK_NULL_HANDLE;
    }
    if (mTextureImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(mDevice, mTextureImageMemory, nullptr);
        mTextureImageMemory = VK_NULL_HANDLE;
    }
    if (mSpriteVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(mDevice, mSpriteVertexBuffer, nullptr);
        mSpriteVertexBuffer = VK_NULL_HANDLE;
    }
    if (mSpriteVertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(mDevice, mSpriteVertexBufferMemory, nullptr);
        mSpriteVertexBufferMemory = VK_NULL_HANDLE;
    }

    for (size_t i = 0; i < mImageAvailableSemaphores.size(); i++) {
        vkDestroySemaphore(mDevice, mImageAvailableSemaphores[i], nullptr);
    }
    for (size_t i = 0; i < mRenderFinishedSemaphores.size(); i++) {
        vkDestroySemaphore(mDevice, mRenderFinishedSemaphores[i], nullptr);
    }
    // تدمير fences (عدد إطارات متوازية)
    for (size_t i = 0; i < mInFlightFences.size(); i++) {
        vkDestroyFence(mDevice, mInFlightFences[i], nullptr);
    }
    mImageAvailableSemaphores.clear();
    mRenderFinishedSemaphores.clear();
    mInFlightFences.clear();
    mImagesInFlight.clear();

    if (mCommandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
        mCommandPool = VK_NULL_HANDLE;
    }
    if (mDevice != VK_NULL_HANDLE) {
        vkDestroyDevice(mDevice, nullptr);
        mDevice = VK_NULL_HANDLE;
    }

    if (ENABLE_VALIDATION_LAYERS) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) func(mInstance, mDebugMessenger, nullptr);
    }

    if (mSurface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
        mSurface = VK_NULL_HANDLE;
    }
    if (mInstance != VK_NULL_HANDLE) {
        vkDestroyInstance(mInstance, nullptr);
        mInstance = VK_NULL_HANDLE;
    }
    mWindowHandle = nullptr;
}

void VulkanContext::cleanupSwapchain() {
    for (auto fb : mSwapchainFramebuffers) {
        vkDestroyFramebuffer(mDevice, fb, nullptr);
    }
    mSwapchainFramebuffers.clear();

    for (auto iv : mSwapchainImageViews) {
        vkDestroyImageView(mDevice, iv, nullptr);
    }
    mSwapchainImageViews.clear();

    cleanupSpritePipeline();

    if (mRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
        mRenderPass = VK_NULL_HANDLE;
    }

    if (mSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
        mSwapchain = VK_NULL_HANDLE;
    }
    mSwapchainImages.clear();
    mImagesInFlight.clear();
}

void VulkanContext::recreateSwapchain() {
    waitForValidFramebufferSize();
    if (mWidth == 0 || mHeight == 0) {
        return;
    }

    vkDeviceWaitIdle(mDevice);

    cleanupSwapchain();

    createSwapchain();
    createImageViews();
    createRenderPass();
    createFramebuffers();
    createSyncObjects();

    if (hasSpriteResources()) {
        createSpritePipeline();
    }

    mCurrentFrame = 0;
    mSwapchainDirty = false;
}

void VulkanContext::onResize(uint32_t newWidth, uint32_t newHeight) {
    mWidth = newWidth;
    mHeight = newHeight;
    mSwapchainDirty = true;
}

bool VulkanContext::isInitialized() const { return mInitialized; }

void VulkanContext::waitIdle() {
    if (mDevice) vkDeviceWaitIdle(mDevice);
}

// ─── إنشاء Instance ─────────────────────────────────────────────

void VulkanContext::createInstance() {
    if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport()) {
        throw std::runtime_error("[Vulkan] طبقات التحقق غير متوفرة");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "DuckH";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (ENABLE_VALIDATION_LAYERS) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء VkInstance");
    }
}

// ─── Debug Messenger ─────────────────────────────────────────────

void VulkanContext::setupDebugMessenger() {
    if (!ENABLE_VALIDATION_LAYERS) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT,
                                     VkDebugUtilsMessageTypeFlagsEXT,
                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                     void*) -> VkBool32 {
        std::cerr << "[Vulkan Validation] " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    };

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");
    if (!func || func(mInstance, &createInfo, nullptr, &mDebugMessenger) != VK_SUCCESS) {
        std::cerr << "[Vulkan] تحذير: فشل إعداد Debug Messenger" << std::endl;
    }
}

// ─── Surface ─────────────────────────────────────────────────────

void VulkanContext::createSurface(GLFWwindow* window) {
    if (glfwCreateWindowSurface(mInstance, window, nullptr, &mSurface) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء Surface");
    }
}

// ─── اختيار الجهاز الفعلي ──────────────────────────────────────

void VulkanContext::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("[Vulkan] لا يوجد GPU يدعم Vulkan");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

    int bestScore = 0;
    for (const auto& device : devices) {
        int score = rateDeviceSuitability(device);
        if (score > bestScore) {
            bestScore = score;
            mPhysicalDevice = device;
        }
    }

    if (mPhysicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("[Vulkan] لا يوجد GPU مناسب");
    }
}

int VulkanContext::rateDeviceSuitability(VkPhysicalDevice device) const {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    int score = 0;

    // GPU منفصل أفضل بكثير من المدمج
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    score += static_cast<int>(props.limits.maxImageDimension2D);

    // يجب أن يدعم الطوابير المطلوبة
    if (!findQueueFamilies(device).isComplete()) return 0;

    // يجب أن يدعم امتدادات السوابتشين
    if (!checkDeviceExtensionSupport(device)) return 0;

    // يجب أن يدعم السوابتشين بشكل كافٍ
    auto swapchainSupport = querySwapchainSupport(device);
    if (swapchainSupport.formats.empty() || swapchainSupport.presentModes.empty()) return 0;

    return score;
}

bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice device) const {
    uint32_t extCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);

    std::vector<VkExtensionProperties> available(extCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, available.data());

    for (const char* required : DEVICE_EXTENSIONS) {
        bool found = false;
        for (const auto& ext : available) {
            if (strcmp(required, ext.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice device) const {
    QueueFamilyIndices indices;

    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);
    std::vector<VkQueueFamilyProperties> queues(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queues.data());

    for (uint32_t i = 0; i < queueCount; i++) {
        if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) break;
    }

    return indices;
}

// ─── الجهاز المنطقي ─────────────────────────────────────────────

void VulkanContext::createLogicalDevice() {
    auto indices = findQueueFamilies(mPhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    std::set<uint32_t> uniqueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo qi{};
        qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.queueFamilyIndex = family;
        qi.queueCount = 1;
        qi.pQueuePriorities = &queuePriority;
        queueInfos.push_back(qi);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
    createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

    if (ENABLE_VALIDATION_LAYERS) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    }

    if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء Logical Device");
    }

    vkGetDeviceQueue(mDevice, indices.graphicsFamily.value(), 0, &mGraphicsQueue);
    vkGetDeviceQueue(mDevice, indices.presentFamily.value(), 0, &mPresentQueue);
}

// ─── السوابتشين ─────────────────────────────────────────────────

SwapchainSupportDetails VulkanContext::querySwapchainSupport(VkPhysicalDevice device) const {
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mSurface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, details.formats.data());
    }

    uint32_t modeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &modeCount, nullptr);
    if (modeCount != 0) {
        details.presentModes.resize(modeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &modeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR VulkanContext::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available) const {
    // نفضّل B8G8R8A8 مع SRGB
    for (const auto& fmt : available) {
        if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
            fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return fmt;
        }
    }
    return available[0];
}

VkPresentModeKHR VulkanContext::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available) const {
    // نفضّل Mailbox (ثلاثي التخزين بدون تمزق)
    for (const auto& mode : available) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR; // مضمون متوفر
}

VkExtent2D VulkanContext::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps, uint32_t width, uint32_t height) const {
    if (caps.currentExtent.width != UINT32_MAX) {
        return caps.currentExtent;
    }

    VkExtent2D extent = { width, height };
    extent.width = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return extent;
}

void VulkanContext::createSwapchain() {
    auto support = querySwapchainSupport(mPhysicalDevice);
    auto format = chooseSwapSurfaceFormat(support.formats);
    auto mode = chooseSwapPresentMode(support.presentModes);
    auto extent = chooseSwapExtent(support.capabilities, mWidth, mHeight);

    // عدد الصور = الحد الأدنى + 1 (لتجنب الانتظار)
    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
        imageCount = support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = mSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = format.format;
    createInfo.imageColorSpace = format.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto indices = findQueueFamilies(mPhysicalDevice);
    uint32_t families[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = families;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = mode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapchain) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء Swapchain");
    }

    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, nullptr);
    mSwapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, mSwapchainImages.data());
    mImagesInFlight.assign(imageCount, VK_NULL_HANDLE);

    mSwapchainImageFormat = format.format;
    mSwapchainExtent = extent;
}

// ─── Image Views ─────────────────────────────────────────────────

void VulkanContext::createImageViews() {
    mSwapchainImageViews.resize(mSwapchainImages.size());

    for (size_t i = 0; i < mSwapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = mSwapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = mSwapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(mDevice, &createInfo, nullptr, &mSwapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("[Vulkan] فشل إنشاء Image View");
        }
    }
}

// ─── Render Pass ─────────────────────────────────────────────────

void VulkanContext::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = mSwapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    // تبعية: انتظار حتى يكون Semaphore جاهزًا قبل البدء
    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments = &colorAttachment;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;
    rpInfo.dependencyCount = 1;
    rpInfo.pDependencies = &dep;

    if (vkCreateRenderPass(mDevice, &rpInfo, nullptr, &mRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء Render Pass");
    }
}

// ─── Framebuffers ────────────────────────────────────────────────

void VulkanContext::createFramebuffers() {
    mSwapchainFramebuffers.resize(mSwapchainImageViews.size());

    for (size_t i = 0; i < mSwapchainImageViews.size(); i++) {
        VkImageView attachments[] = { mSwapchainImageViews[i] };

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = mRenderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = attachments;
        fbInfo.width = mSwapchainExtent.width;
        fbInfo.height = mSwapchainExtent.height;
        fbInfo.layers = 1;

        if (vkCreateFramebuffer(mDevice, &fbInfo, nullptr, &mSwapchainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("[Vulkan] فشل إنشاء Framebuffer");
        }
    }
}

// ─── Command Pool ────────────────────────────────────────────────

void VulkanContext::createCommandPool() {
    auto indices = findQueueFamilies(mPhysicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء Command Pool");
    }
}

// ─── Vertex Buffer (مثلث ملون) ──────────────────────────────────

// ─── Command Buffers ─────────────────────────────────────────────

void VulkanContext::createCommandBuffers() {
    if (!mCommandBuffers.empty()) {
        vkFreeCommandBuffers(
            mDevice,
            mCommandPool,
            static_cast<uint32_t>(mCommandBuffers.size()),
            mCommandBuffers.data()
        );
        mCommandBuffers.clear();
    }

    mCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(mCommandBuffers.size());

    if (vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء Command Buffers");
    }
}

// ─── Sync Objects ────────────────────────────────────────────────

void VulkanContext::createSyncObjects() {
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (mImageAvailableSemaphores.empty()) {
        mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(mDevice, &semInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS) {
                throw std::runtime_error("[Vulkan] فشل إنشاء ImageAvailable Semaphores");
            }
        }
    }

    if (mInFlightFences.empty()) {
        mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("[Vulkan] فشل إنشاء Fences");
            }
        }
    }

    for (VkSemaphore semaphore : mRenderFinishedSemaphores) {
        vkDestroySemaphore(mDevice, semaphore, nullptr);
    }
    mRenderFinishedSemaphores.clear();
    mRenderFinishedSemaphores.resize(mSwapchainImages.size());

    for (size_t i = 0; i < mRenderFinishedSemaphores.size(); i++) {
        if (vkCreateSemaphore(mDevice, &semInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("[Vulkan] فشل إنشاء RenderFinished Semaphores");
        }
    }

    mImagesInFlight.assign(mSwapchainImages.size(), VK_NULL_HANDLE);
}

// ─── رسم إطار ───────────────────────────────────────────────────

void VulkanContext::drawFrame() {
    if (mSwapchainDirty && mWidth > 0 && mHeight > 0) {
        recreateSwapchain();
    }

    if (!mInitialized || mWidth == 0 || mHeight == 0) return;
    if (mSwapchain == VK_NULL_HANDLE || mRenderPass == VK_NULL_HANDLE || mCommandBuffers.empty()) return;

    // انتظار انتهاء الإطار السابق
    vkWaitForFences(mDevice, 1, &mInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

    // الحصول على صورة من السوابتشين
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        mDevice, mSwapchain, UINT64_MAX,
        mImageAvailableSemaphores[mCurrentFrame],
        VK_NULL_HANDLE, &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        mSwapchainDirty = true;
        recreateSwapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("[Vulkan] فشل الحصول على صورة Swapchain");
    }

    if (imageIndex >= mImagesInFlight.size()) {
        throw std::runtime_error("[Vulkan] imageIndex خارج نطاق swapchain tracking");
    }

    if (mImagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(mDevice, 1, &mImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    mImagesInFlight[imageIndex] = mInFlightFences[mCurrentFrame];

    // إعادة ضبط الـ fence
    vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrame]);

    // تسجيل أوامر الرسم

    // تسجيل أوامر الرسم
    vkResetCommandBuffer(mCommandBuffers[mCurrentFrame], 0);
    {
        VkCommandBuffer cmd = mCommandBuffers[mCurrentFrame];

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(cmd, &beginInfo);

        VkRenderPassBeginInfo rpBegin{};
        rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin.renderPass = mRenderPass;
        rpBegin.framebuffer = mSwapchainFramebuffers[imageIndex];
        rpBegin.renderArea.offset = { 0, 0 };
        rpBegin.renderArea.extent = mSwapchainExtent;

        // لون الخلفية: أزرق داكن يشبه السماء
        VkClearValue clearColor = { {{0.05f, 0.05f, 0.15f, 1.0f}} };
        rpBegin.clearValueCount = 1;
        rpBegin.pClearValues = &clearColor;

        vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

        // رسم السبرايت (إذا كان النسيج محمّلًا)
        if (mSpritePipeline != VK_NULL_HANDLE && mSpriteVertexBuffer != VK_NULL_HANDLE) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mSpritePipeline);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(mSwapchainExtent.width);
            viewport.height = static_cast<float>(mSwapchainExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(cmd, 0, 1, &viewport);

            VkRect2D scissor{{0, 0}, mSwapchainExtent};
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                mSpritePipelineLayout, 0, 1, &mDescriptorSet, 0, nullptr);

            VkBuffer spriteVB[] = { mSpriteVertexBuffer };
            VkDeviceSize spriteOff[] = { 0 };
            vkCmdBindVertexBuffers(cmd, 0, 1, spriteVB, spriteOff);

            vkCmdDraw(cmd, 6, 1, 0, 0);
        }

        vkCmdEndRenderPass(cmd);

        if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
            throw std::runtime_error("[Vulkan] فشل تسجيل Command Buffer");
        }
    }

    // إرسال الأوامر
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { mImageAvailableSemaphores[mCurrentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffers[mCurrentFrame];

    // كل صورة swapchain لها render-finished semaphore خاصة بها لتجنّب إعادة استخدامها قبل إعادة acquire.
    VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphores[imageIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurrentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إرسال أوامر الرسم");
    }

    // عرض الإطار
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapchains[] = { mSwapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(mPresentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mSwapchainDirty) {
        mSwapchainDirty = true;
        recreateSwapchain();
    }

    mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanContext::cleanupSpritePipeline() {
    if (mSpritePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(mDevice, mSpritePipeline, nullptr);
        mSpritePipeline = VK_NULL_HANDLE;
    }
    if (mSpritePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(mDevice, mSpritePipelineLayout, nullptr);
        mSpritePipelineLayout = VK_NULL_HANDLE;
    }
}

void VulkanContext::cleanupSpriteDescriptors() {
    if (mDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
        mDescriptorPool = VK_NULL_HANDLE;
    }
    if (mDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
        mDescriptorSetLayout = VK_NULL_HANDLE;
    }
    mDescriptorSet = VK_NULL_HANDLE;
}

void VulkanContext::waitForValidFramebufferSize() {
    if (mWindowHandle == nullptr) {
        return;
    }

    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(mWindowHandle, &fbWidth, &fbHeight);

    while (fbWidth == 0 || fbHeight == 0) {
        if (glfwWindowShouldClose(mWindowHandle)) {
            mWidth = 0;
            mHeight = 0;
            return;
        }

        glfwWaitEvents();
        glfwGetFramebufferSize(mWindowHandle, &fbWidth, &fbHeight);
    }

    mWidth = static_cast<uint32_t>(fbWidth);
    mHeight = static_cast<uint32_t>(fbHeight);
}

bool VulkanContext::hasSpriteResources() const {
    return mTextureImageView != VK_NULL_HANDLE &&
           mTextureSampler != VK_NULL_HANDLE &&
           mDescriptorSetLayout != VK_NULL_HANDLE &&
           mDescriptorSet != VK_NULL_HANDLE;
}

// ─── أدوات مساعدة ───────────────────────────────────────────────

std::vector<const char*> VulkanContext::getRequiredExtensions() const {
    uint32_t glfwCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwCount);
    std::vector<const char*> extensions(glfwExts, glfwExts + glfwCount);

    if (ENABLE_VALIDATION_LAYERS) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

bool VulkanContext::checkValidationLayerSupport() const {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> available(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, available.data());

    for (const char* name : VALIDATION_LAYERS) {
        bool found = false;
        for (const auto& layer : available) {
            if (strcmp(name, layer.layerName) == 0) { found = true; break; }
        }
        if (!found) return false;
    }
    return true;
}

VkShaderModule VulkanContext::createShaderModule(const std::vector<char>& code) const {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule module;
    if (vkCreateShaderModule(mDevice, &createInfo, nullptr, &module) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء Shader Module");
    }
    return module;
}

std::vector<char> VulkanContext::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("[Vulkan] فشل قراءة ملف: " + path);
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    file.close();
    return buffer;
}

// ─── تحميل النسيج ───────────────────────────────────────────────

void VulkanContext::loadTexture(const std::string& path) {
    createTextureImage(path);
    createTextureImageView();
    createTextureSampler();
    createSpriteDescriptors();
    createSpritePipeline();
    createCommandBuffers();
}

void VulkanContext::loadTextureWithCallback(const std::string& path, PixelCallback cb) {
    int texW = 0, texH = 0, texChannels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &texW, &texH, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("[Vulkan] فشل تحميل الصورة: " + path);
    }

    // تمرير البكسلات لـ callback قبل إنشاء موارد Vulkan
    if (cb) {
        cb(texW, texH, pixels);
    }

    // إنشاء موارد Vulkan من نفس البكسلات
    createTextureImageFromPixels(pixels, texW, texH);
    stbi_image_free(pixels);

    createTextureImageView();
    createTextureSampler();
    createSpriteDescriptors();
    createSpritePipeline();
    createCommandBuffers();
}

void VulkanContext::updateSpriteQuad(float u0, float u1, float v0, float v1,
                                     float x0, float x1, float y0, float y1) {
    // quad = مثلثين (6 رؤوس): position(vec2) + texCoord(vec2)
    float vertices[] = {
        // مثلث أول
        x0, y0,  u0, v0,  // أعلى يسار
        x1, y0,  u1, v0,  // أعلى يمين
        x1, y1,  u1, v1,  // أسفل يمين
        // مثلث ثاني
        x0, y0,  u0, v0,
        x1, y1,  u1, v1,
        x0, y1,  u0, v1,
    };

    void* data;
    vkMapMemory(mDevice, mSpriteVertexBufferMemory, 0, sizeof(vertices), 0, &data);
    memcpy(data, vertices, sizeof(vertices));
    vkUnmapMemory(mDevice, mSpriteVertexBufferMemory);
}

void VulkanContext::createTextureImage(const std::string& path) {
    int texW, texH, texChannels;
    stbi_uc* pixels = stbi_load(path.c_str(), &texW, &texH, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("[Vulkan] فشل تحميل الصورة: " + path);
    }

    createTextureImageFromPixels(pixels, texW, texH);
    stbi_image_free(pixels);
}

void VulkanContext::createTextureImageFromPixels(const unsigned char* pixels, int texW, int texH) {
    VkDeviceSize imageSize = VkDeviceSize(texW) * texH * 4;
    mMipLevels = 1;

    if (mTextureImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(mDevice, mTextureImageView, nullptr);
        mTextureImageView = VK_NULL_HANDLE;
    }
    if (mTextureImage != VK_NULL_HANDLE) {
        vkDestroyImage(mDevice, mTextureImage, nullptr);
        mTextureImage = VK_NULL_HANDLE;
    }
    if (mTextureImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(mDevice, mTextureImageMemory, nullptr);
        mTextureImageMemory = VK_NULL_HANDLE;
    }

    // إنشاء buffer مؤقت لنسخ البيانات
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    createBuffer(imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(mDevice, stagingMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(mDevice, stagingMemory);

    // إنشاء الصورة
    VkImageCreateInfo imgInfo{};
    imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.extent.width = static_cast<uint32_t>(texW);
    imgInfo.extent.height = static_cast<uint32_t>(texH);
    imgInfo.extent.depth = 1;
    imgInfo.mipLevels = mMipLevels;
    imgInfo.arrayLayers = 1;
    imgInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(mDevice, &imgInfo, nullptr, &mTextureImage) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء صورة النسيج");
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(mDevice, mTextureImage, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vkAllocateMemory(mDevice, &allocInfo, nullptr, &mTextureImageMemory);
    vkBindImageMemory(mDevice, mTextureImage, mTextureImageMemory, 0);

    transitionImageLayout(mTextureImage, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, mTextureImage,
        static_cast<uint32_t>(texW), static_cast<uint32_t>(texH));
    transitionImageLayout(mTextureImage, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
    vkFreeMemory(mDevice, stagingMemory, nullptr);

    std::cout << "[Vulkan] تم تحميل النسيج: " << texW << "x" << texH << std::endl;
}

void VulkanContext::createTextureImageView() {
    if (mTextureImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(mDevice, mTextureImageView, nullptr);
        mTextureImageView = VK_NULL_HANDLE;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = mTextureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mMipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(mDevice, &viewInfo, nullptr, &mTextureImageView) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء Image View للنسيج");
    }
}

void VulkanContext::createTextureSampler() {
    if (mTextureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(mDevice, mTextureSampler, nullptr);
        mTextureSampler = VK_NULL_HANDLE;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // هذا الأطلس بلا padding بين الخلايا، لذا nearest يمنع تسرب الفريمات المتجاورة.
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mMipLevels);

    if (vkCreateSampler(mDevice, &samplerInfo, nullptr, &mTextureSampler) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء Sampler");
    }
}

void VulkanContext::createSpriteDescriptors() {
    cleanupSpritePipeline();
    cleanupSpriteDescriptors();

    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.pImmutableSamplers = nullptr;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;

    if (vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء Descriptor Set Layout");
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء Descriptor Pool");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = mDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &mDescriptorSetLayout;

    if (vkAllocateDescriptorSets(mDevice, &allocInfo, &mDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("[Vulkan] فشل إنشاء Descriptor Set");
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = mTextureImageView;
    imageInfo.sampler = mTextureSampler;

    VkWriteDescriptorSet descWrite{};
    descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descWrite.dstSet = mDescriptorSet;
    descWrite.dstBinding = 0;
    descWrite.dstArrayElement = 0;
    descWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descWrite.descriptorCount = 1;
    descWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(mDevice, 1, &descWrite, 0, nullptr);
}

// ─── Pipeline السبرايت (مع Texture) ─────────────────────────────

void VulkanContext::createSpritePipeline() {
    if (mRenderPass == VK_NULL_HANDLE || mDescriptorSetLayout == VK_NULL_HANDLE) {
        throw std::runtime_error("[Vulkan] لا يمكن إنشاء Sprite Pipeline قبل تجهيز Render Pass و Descriptor Layout");
    }

    cleanupSpritePipeline();

    // إنشاء Pipeline
    auto vertCode = readFile("assets/shaders/vert/sprite.vert.spv");
    auto fragCode = readFile("assets/shaders/frag/sprite.frag.spv");
    VkShaderModule vertMod = createShaderModule(vertCode);
    VkShaderModule fragMod = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo stages[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertMod, "main", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragMod, "main", nullptr}
    };

    // Vertex: position(vec2) + texCoord(vec2)
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = 4 * sizeof(float);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0].binding = 0; attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32_SFLOAT; attrs[0].offset = 0;
    attrs[1].binding = 0; attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R32G32_SFLOAT; attrs[1].offset = 2 * sizeof(float);

    VkPipelineVertexInputStateCreateInfo vertInput{};
    vertInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInput.vertexBindingDescriptionCount = 1;
    vertInput.pVertexBindingDescriptions = &binding;
    vertInput.vertexAttributeDescriptionCount = 2;
    vertInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport vp{};
    vp.width = static_cast<float>(mSwapchainExtent.width);
    vp.height = static_cast<float>(mSwapchainExtent.height);
    vp.maxDepth = 1.0f;

    VkRect2D scissor{{0, 0}, mSwapchainExtent};

    VkPipelineViewportStateCreateInfo vpState{};
    vpState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpState.viewportCount = 1;
    vpState.pViewports = &vp;
    vpState.scissorCount = 1;
    vpState.pScissors = &scissor;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.lineWidth = 1.0f;
    raster.cullMode = VK_CULL_MODE_NONE; // لا إخفاء - الـ quad ثنائي الأبعاد
    raster.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttach.blendEnable = VK_TRUE; // دمج ألفا للشفافية
    blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttach.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttach.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blendState{};
    blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendState.attachmentCount = 1;
    blendState.pAttachments = &blendAttach;

    VkPipelineLayoutCreateInfo pipeLayout{};
    pipeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLayout.setLayoutCount = 1;
    pipeLayout.pSetLayouts = &mDescriptorSetLayout;

    if (vkCreatePipelineLayout(mDevice, &pipeLayout, nullptr, &mSpritePipelineLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(mDevice, fragMod, nullptr);
        vkDestroyShaderModule(mDevice, vertMod, nullptr);
        throw std::runtime_error("[Vulkan] فشل إنشاء Pipeline Layout للسبرايت");
    }

    VkGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeInfo.stageCount = 2;
    pipeInfo.pStages = stages;
    pipeInfo.pVertexInputState = &vertInput;
    pipeInfo.pInputAssemblyState = &assembly;
    pipeInfo.pViewportState = &vpState;
    pipeInfo.pRasterizationState = &raster;
    pipeInfo.pMultisampleState = &ms;
    pipeInfo.pColorBlendState = &blendState;
    pipeInfo.pDynamicState = &dynamicState;
    pipeInfo.layout = mSpritePipelineLayout;
    pipeInfo.renderPass = mRenderPass;
    pipeInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &mSpritePipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(mDevice, fragMod, nullptr);
        vkDestroyShaderModule(mDevice, vertMod, nullptr);
        throw std::runtime_error("[Vulkan] فشل إنشاء Graphics Pipeline للسبرايت");
    }

    vkDestroyShaderModule(mDevice, fragMod, nullptr);
    vkDestroyShaderModule(mDevice, vertMod, nullptr);
}

void VulkanContext::createSpriteVertexBuffer() {
    // 6 رؤوس (مثلثين) = quad كامل - UV افتراضية (كل النسيج)
    float vertices[] = {
        -0.4f, -0.6f,  0.0f, 0.0f,
         0.4f, -0.6f,  1.0f, 0.0f,
         0.4f,  0.6f,  1.0f, 1.0f,
        -0.4f, -0.6f,  0.0f, 0.0f,
         0.4f,  0.6f,  1.0f, 1.0f,
        -0.4f,  0.6f,  0.0f, 1.0f,
    };

    VkDeviceSize bufferSize = sizeof(vertices);
    createBuffer(bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        mSpriteVertexBuffer, mSpriteVertexBufferMemory);

    void* data;
    vkMapMemory(mDevice, mSpriteVertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, bufferSize);
    vkUnmapMemory(mDevice, mSpriteVertexBufferMemory);
}

// ─── أدوات Buffer/Image ─────────────────────────────────────────

uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) const {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    throw std::runtime_error("[Vulkan] لم يُعثر على نوع ذاكرة مناسب");
}

void VulkanContext::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                  VkMemoryPropertyFlags props,
                                  VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size = size;
    bufInfo.usage = usage;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(mDevice, &bufInfo, nullptr, &buffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(mDevice, buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, props);

    vkAllocateMemory(mDevice, &allocInfo, nullptr, &memory);
    vkBindBufferMemory(mDevice, buffer, memory, 0);
}

void VulkanContext::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = mCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(mDevice, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &copyRegion);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mGraphicsQueue);

    vkFreeCommandBuffers(mDevice, mCommandPool, 1, &cmd);
}

void VulkanContext::transitionImageLayout(VkImage image, VkFormat /*format*/,
                                           VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = mCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(mDevice, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mMipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage, dstStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::runtime_error("[Vulkan] انتقال تخطيط غير مدعوم");
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mGraphicsQueue);
    vkFreeCommandBuffers(mDevice, mCommandPool, 1, &cmd);
}

void VulkanContext::copyBufferToImage(VkBuffer buffer, VkImage image,
                                       uint32_t texWidth, uint32_t texHeight) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = mCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(mDevice, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {texWidth, texHeight, 1};

    vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mGraphicsQueue);
    vkFreeCommandBuffers(mDevice, mCommandPool, 1, &cmd);
}

} // namespace gfx
