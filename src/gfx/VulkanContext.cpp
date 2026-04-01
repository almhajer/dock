#include "VulkanContext.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const std::vector<const char *> gfx::VulkanContext::VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char *> gfx::VulkanContext::DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

namespace
{
struct WindUniformData
{
    alignas(16) std::array<float, 4> motion = {0.0f, 0.035f, 1.4f, 0.1f};
    alignas(16) std::array<float, 4> wind = {1.0f, 0.35f, 0.0f, 0.0f};
    alignas(16) std::array<float, 4> interactionA = {0.0f, 0.0f, 0.0f, 0.0f};
    alignas(16) std::array<float, 4> interactionB = {0.0f, 0.0f, 0.0f, 0.0f};
};

constexpr gfx::SunDiskUniformData kDefaultSunDiskUniformData = {
    {0.82f, 0.20f, 0.022f, 0.065f},
    {1.0f, 1.0f, 1.0f, 2.2f},
    {0.50f, 8.0f, 0.04f, 0.0f},
};

constexpr gfx::GodRayUniformData kDefaultGodRayUniformData = {
    {0.82f, 0.20f, 0.92f, 0.018f},
    {0.96f, 0.45f, 0.35f, 28.0f},
};

constexpr float PI = 3.14159265358979323846f;

std::vector<unsigned char> generateWindTexturePixels(int width, int height)
{
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 255u);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const float u = static_cast<float>(x) / static_cast<float>(width);
            const float v = static_cast<float>(y) / static_cast<float>(height);

            const float angleA = std::sin(2.0f * PI * (u * 1.0f + v * 0.35f));
            const float angleB = std::cos(2.0f * PI * (u * 0.5f - v * 1.3f));
            const float flowX = std::sin(2.0f * PI * (u * 1.2f + v * 0.18f)) +
                                0.45f * std::sin(2.0f * PI * (u * 2.1f - v * 0.9f)) +
                                0.25f * angleB;
            const float flowY = std::cos(2.0f * PI * (v * 1.1f - u * 0.22f)) +
                                0.45f * std::cos(2.0f * PI * (u * 1.6f + v * 1.8f)) +
                                0.25f * angleA;

            const float length = std::sqrt(flowX * flowX + flowY * flowY);
            const float invLength = (length > 0.0001f) ? 1.0f / length : 1.0f;
            const float dirX = flowX * invLength;
            const float dirY = flowY * invLength;

            const float strength =
                0.5f + 0.5f *
                (0.55f * std::sin(2.0f * PI * (u * 0.8f + v * 1.4f)) +
                 0.45f * std::cos(2.0f * PI * (u * 1.7f - v * 0.6f)));

            const std::size_t index =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
            pixels[index + 0] = static_cast<unsigned char>(std::clamp(dirX * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f);
            pixels[index + 1] = static_cast<unsigned char>(std::clamp(dirY * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f);
            pixels[index + 2] = static_cast<unsigned char>(std::clamp(strength, 0.0f, 1.0f) * 255.0f);
            pixels[index + 3] = 255u;
        }
    }

    return pixels;
}

std::vector<unsigned char> generateSunMaskPixels(int width, int height, const gfx::SunDiskUniformData& sunData)
{
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 0u);

    const float sunX = sunData.sunDisk[0];
    const float sunY = sunData.sunDisk[1];
    const float diskRadius = sunData.sunDisk[2];
    const float coronaRadius = sunData.sunDisk[3];
    const float falloff = sunData.appearance[1];

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
            const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
            const float dx = u - sunX;
            const float dy = v - sunY;
            const float distanceToSun = std::sqrt(dx * dx + dy * dy);
            const float angle = std::atan2(dy, dx);

            const float diskMask = 1.0f - std::clamp((distanceToSun - diskRadius * 0.92f) / std::max(diskRadius * 0.16f, 0.0001f), 0.0f, 1.0f);
            const float coronaT = std::clamp((distanceToSun - diskRadius) / std::max(coronaRadius - diskRadius, 0.0001f), 0.0f, 1.0f);
            const float corona = std::exp(-coronaT * falloff) * (distanceToSun >= diskRadius ? 1.0f : 0.0f);
            const float primary = std::pow(std::max(0.0f, std::cos(angle * 6.0f)), 10.0f);
            const float secondary = std::pow(std::max(0.0f, std::cos(angle * 12.0f + 0.25f)), 18.0f) * 0.5f;
            const float diagonal = std::pow(std::max(0.0f, std::cos(angle * 4.0f - 0.18f)), 14.0f) * 0.35f;
            const float burst = (primary + secondary + diagonal) *
                                (1.0f - std::exp(-distanceToSun / std::max(coronaRadius * 0.18f, 0.0001f))) *
                                std::exp(-distanceToSun / std::max(coronaRadius * 0.95f, 0.0001f));
            const float value = std::clamp(std::max(std::max(diskMask, corona * 0.70f), burst), 0.0f, 1.0f);

            const std::size_t index =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
            const unsigned char byteValue = static_cast<unsigned char>(value * 255.0f);
            pixels[index + 0] = byteValue;
            pixels[index + 1] = byteValue;
            pixels[index + 2] = byteValue;
            pixels[index + 3] = 255u;
        }
    }

    return pixels;
}
} // namespace

namespace gfx
{

VulkanContext::~VulkanContext()
{
    cleanup();
}

void VulkanContext::init(GLFWwindow *window, uint32_t width, uint32_t height)
{
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
    createCommandBuffers();
    createWindUniformBuffers();
    createWindTextureResources();
    createDescriptorSetLayout();
    createSpritePipeline();
    createSyncObjects();
    mAtmosphereRenderer.init(mDevice, mSwapchainExtent);
    createAtmosphereUniformBuffers();
    createAtmosphereVertexBuffer();
    createAtmosphereMaskTexture();
    createAtmosphereDescriptorSets();
    createAtmospherePipelines();

    mInitialized = true;
    std::cout << "[Vulkan] تمت التهيئة بنجاح" << std::endl;
}

void VulkanContext::cleanup()
{
    if (!mInitialized)
    {
        return;
    }

    mInitialized = false;
    mSwapchainDirty = false;

    vkDeviceWaitIdle(mDevice);

    cleanupSwapchain();

    for (auto &layer : mSpriteLayers)
    {
        destroyLayerResources(layer);
    }
    mSpriteLayers.clear();
    destroyAtmosphereDescriptorSets();
    destroyAtmosphereMaskTexture();
    destroyAtmosphereVertexBuffer();
    destroyAtmosphereUniformBuffers();
    destroyWindTextureResources();
    destroyWindUniformBuffers();
    mAtmosphereRenderer.cleanup();

    if (mDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
        mDescriptorSetLayout = VK_NULL_HANDLE;
    }

    for (VkSemaphore semaphore : mImageAvailableSemaphores)
    {
        vkDestroySemaphore(mDevice, semaphore, nullptr);
    }
    for (VkSemaphore semaphore : mRenderFinishedSemaphores)
    {
        vkDestroySemaphore(mDevice, semaphore, nullptr);
    }
    for (VkFence fence : mInFlightFences)
    {
        vkDestroyFence(mDevice, fence, nullptr);
    }
    mImageAvailableSemaphores.clear();
    mRenderFinishedSemaphores.clear();
    mInFlightFences.clear();
    mImagesInFlight.clear();

    if (mCommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
        mCommandPool = VK_NULL_HANDLE;
    }

    if (mDevice != VK_NULL_HANDLE)
    {
        vkDestroyDevice(mDevice, nullptr);
        mDevice = VK_NULL_HANDLE;
    }

    if (ENABLE_VALIDATION_LAYERS && mDebugMessenger != VK_NULL_HANDLE)
    {
        auto destroyDebugMessenger =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroyDebugMessenger != nullptr)
        {
            destroyDebugMessenger(mInstance, mDebugMessenger, nullptr);
        }
        mDebugMessenger = VK_NULL_HANDLE;
    }

    if (mSurface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
        mSurface = VK_NULL_HANDLE;
    }

    if (mInstance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(mInstance, nullptr);
        mInstance = VK_NULL_HANDLE;
    }

    mWindowHandle = nullptr;
}

void VulkanContext::cleanupSwapchain()
{
    cleanupAtmospherePipelines();
    cleanupSpritePipeline();

    for (VkFramebuffer framebuffer : mSwapchainFramebuffers)
    {
        vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
    }
    mSwapchainFramebuffers.clear();

    for (VkImageView imageView : mSwapchainImageViews)
    {
        vkDestroyImageView(mDevice, imageView, nullptr);
    }
    mSwapchainImageViews.clear();

    if (mRenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
        mRenderPass = VK_NULL_HANDLE;
    }

    if (mSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
        mSwapchain = VK_NULL_HANDLE;
    }

    mSwapchainImages.clear();
    mImagesInFlight.clear();
}

void VulkanContext::recreateSwapchain()
{
    waitForValidFramebufferSize();
    if (mWidth == 0 || mHeight == 0)
    {
        return;
    }

    vkDeviceWaitIdle(mDevice);

    cleanupSwapchain();

    createSwapchain();
    createImageViews();
    createRenderPass();
    createFramebuffers();
    createSpritePipeline();
    createSyncObjects();
    mAtmosphereRenderer.onResize(mSwapchainExtent);
    createAtmospherePipelines();

    mCurrentFrame = 0;
    mSwapchainDirty = false;
}

void VulkanContext::onResize(uint32_t newWidth, uint32_t newHeight)
{
    mWidth = newWidth;
    mHeight = newHeight;
    mSwapchainDirty = true;
}

void VulkanContext::drawFrame()
{
    if (!mInitialized)
    {
        return;
    }

    if (mSwapchainDirty && mWidth > 0 && mHeight > 0)
    {
        recreateSwapchain();
    }

    if (mWidth == 0 || mHeight == 0)
    {
        return;
    }

    if (mSwapchain == VK_NULL_HANDLE || mRenderPass == VK_NULL_HANDLE || mCommandBuffers.empty())
    {
        return;
    }

    vkWaitForFences(mDevice, 1, &mInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        mDevice,
        mSwapchain,
        UINT64_MAX,
        mImageAvailableSemaphores[mCurrentFrame],
        VK_NULL_HANDLE,
        &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        mSwapchainDirty = true;
        recreateSwapchain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("[Vulkan] فشل الحصول على صورة Swapchain");
    }

    if (imageIndex >= mImagesInFlight.size())
    {
        throw std::runtime_error("[Vulkan] imageIndex خارج نطاق swapchain tracking");
    }

    if (mImagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(mDevice, 1, &mImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    mImagesInFlight[imageIndex] = mInFlightFences[mCurrentFrame];

    vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrame]);
    updateWindUniformBuffer(mCurrentFrame);

    {
        const float time = static_cast<float>(glfwGetTime());
        const float sunOffsetX = std::sin(time * 0.07f) * 0.012f;
        const float sunOffsetY = std::sin(time * 0.11f + 1.5f) * 0.006f;

        if (mSunUniformBuffersMapped[mCurrentFrame] != nullptr)
        {
            gfx::SunDiskUniformData sunData = kDefaultSunDiskUniformData;
            sunData.sunDisk[0] += sunOffsetX;
            sunData.sunDisk[1] += sunOffsetY;
            std::memcpy(mSunUniformBuffersMapped[mCurrentFrame], &sunData, sizeof(sunData));
        }
        if (mGodRayUniformBuffersMapped[mCurrentFrame] != nullptr)
        {
            gfx::GodRayUniformData godRayData = kDefaultGodRayUniformData;
            godRayData.sunPosDensityWeight[0] += sunOffsetX;
            godRayData.sunPosDensityWeight[1] += sunOffsetY;
            std::memcpy(mGodRayUniformBuffersMapped[mCurrentFrame], &godRayData, sizeof(godRayData));
        }
    }
    vkResetCommandBuffer(mCommandBuffers[mCurrentFrame], 0);

    VkCommandBuffer cmd = mCommandBuffers[mCurrentFrame];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل بدء Command Buffer");
    }

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = mRenderPass;
    renderPassBeginInfo.framebuffer = mSwapchainFramebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = mSwapchainExtent;

    VkClearValue clearColor = {{{0.05f, 0.05f, 0.15f, 1.0f}}};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

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

    if (mAtmosphereGodRayPipeline != VK_NULL_HANDLE &&
        mAtmosphereRenderer.isInitialized() &&
        mAtmosphereGodRayDescriptorSets[mCurrentFrame] != VK_NULL_HANDLE &&
        mAtmosphereVertexBuffer != VK_NULL_HANDLE)
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mAtmosphereGodRayPipeline);
        const AtmospherePassLayouts& layouts = mAtmosphereRenderer.getLayouts();
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            layouts.godRayPipelineLayout,
            0,
            1,
            &mAtmosphereGodRayDescriptorSets[mCurrentFrame],
            0,
            nullptr);
        VkBuffer vertexBuffers[] = {mAtmosphereVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }

    if (mAtmosphereSunPipeline != VK_NULL_HANDLE &&
        mAtmosphereRenderer.isInitialized() &&
        mAtmosphereSunDescriptorSets[mCurrentFrame] != VK_NULL_HANDLE &&
        mAtmosphereVertexBuffer != VK_NULL_HANDLE)
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mAtmosphereSunPipeline);
        const AtmospherePassLayouts& layouts = mAtmosphereRenderer.getLayouts();
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            layouts.sunPipelineLayout,
            0,
            1,
            &mAtmosphereSunDescriptorSets[mCurrentFrame],
            0,
            nullptr);
        VkBuffer vertexBuffers[] = {mAtmosphereVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }

    if (mSpritePipeline != VK_NULL_HANDLE && hasSpriteResources())
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mSpritePipeline);

        for (const auto &layer : mSpriteLayers)
        {
            if (layer.vertexCount == 0 || layer.vertexBuffer == VK_NULL_HANDLE ||
                layer.descriptorSets.empty() || layer.descriptorSets[mCurrentFrame] == VK_NULL_HANDLE)
            {
                continue;
            }

            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                mSpritePipelineLayout,
                0,
                1,
                &layer.descriptorSets[mCurrentFrame],
                0,
                nullptr);

            VkBuffer vertexBuffers[] = {layer.vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdDraw(cmd, layer.vertexCount, 1, 0, 0);
        }
    }

    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنهاء Command Buffer");
    }

    VkSemaphore waitSemaphores[] = {mImageAvailableSemaphores[mCurrentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {mRenderFinishedSemaphores[imageIndex]};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurrentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إرسال أوامر الرسم");
    }

    VkSwapchainKHR swapchains[] = {mSwapchain};
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(mPresentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mSwapchainDirty)
    {
        mSwapchainDirty = true;
        recreateSwapchain();
    }

    mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanContext::waitIdle()
{
    if (mDevice != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(mDevice);
    }
}

bool VulkanContext::isInitialized() const
{
    return mInitialized;
}

VulkanContext::LayerId VulkanContext::createTexturedLayer(const std::string &path, std::size_t maxQuads)
{
    return createTexturedLayerWithCallback(path, maxQuads, {});
}

VulkanContext::LayerId VulkanContext::createTexturedLayerFromPixels(const unsigned char *pixels,
                                                                    int texW,
                                                                    int texH,
                                                                    std::size_t maxQuads)
{
    return createLayerFromPixels(pixels, texW, texH, maxQuads);
}

VulkanContext::LayerId VulkanContext::createTexturedLayerWithCallback(const std::string &path,
                                                                      std::size_t maxQuads,
                                                                      PixelCallback cb)
{
    if (!mInitialized)
    {
        throw std::runtime_error("[Vulkan] لا يمكن إنشاء طبقة قبل init()");
    }
    if (maxQuads == 0)
    {
        throw std::runtime_error("[Vulkan] maxQuads يجب أن يكون أكبر من صفر");
    }

    int texW = 0;
    int texH = 0;
    int texChannels = 0;
    stbi_uc *pixels = stbi_load(path.c_str(), &texW, &texH, &texChannels, STBI_rgb_alpha);
    if (pixels == nullptr)
    {
        throw std::runtime_error("[Vulkan] فشل تحميل الصورة: " + path);
    }

    if (cb)
    {
        cb(texW, texH, pixels);
    }

    const LayerId layerId = createLayerFromPixels(pixels, texW, texH, maxQuads);
    stbi_image_free(pixels);
    return layerId;
}

VulkanContext::LayerId VulkanContext::createLayerFromPixels(const unsigned char *pixels,
                                                            int texW,
                                                            int texH,
                                                            std::size_t maxQuads)
{
    if (!mInitialized)
    {
        throw std::runtime_error("[Vulkan] لا يمكن إنشاء طبقة قبل init()");
    }
    if (pixels == nullptr || texW <= 0 || texH <= 0)
    {
        throw std::runtime_error("[Vulkan] بيانات الصورة غير صالحة");
    }
    if (maxQuads == 0)
    {
        throw std::runtime_error("[Vulkan] maxQuads يجب أن يكون أكبر من صفر");
    }

    SpriteLayerResources layer;
    layer.maxQuads = maxQuads;

    createLayerTextureImageFromPixels(layer, pixels, texW, texH);
    createLayerTextureImageView(layer);
    createLayerTextureSampler(layer);
    createLayerDescriptors(layer);
    createLayerVertexBuffer(layer);

    mSpriteLayers.push_back(layer);
    return mSpriteLayers.size() - 1;
}

void VulkanContext::updateTexturedLayer(LayerId layerId, const std::vector<TexturedQuad> &quads)
{
    SpriteLayerResources &layer = getLayer(layerId);

    if (layer.vertexBufferMapped == nullptr)
    {
        return;
    }

    if (quads.size() > layer.maxQuads)
    {
        throw std::runtime_error("[Vulkan] عدد quads يتجاوز السعة المخصصة للطبقة");
    }

    std::vector<QuadVertex> vertices;
    vertices.reserve(quads.size() * QUAD_VERTEX_COUNT);
    for (const TexturedQuad &quad : quads)
    {
        const auto quadVertices = buildQuadVertices(quad);
        vertices.insert(vertices.end(), quadVertices.begin(), quadVertices.end());
    }

    const VkDeviceSize byteCount = sizeof(QuadVertex) * vertices.size();
    if (byteCount > 0)
    {
        std::memcpy(layer.vertexBufferMapped, vertices.data(), static_cast<std::size_t>(byteCount));
    }
    layer.vertexCount = static_cast<uint32_t>(vertices.size());
}

void VulkanContext::setGroundInteraction(float leftFootX,
                                         float rightFootX,
                                         float radius,
                                         float leftPressure,
                                         float rightPressure)
{
    mGroundInteractionA = {leftFootX, radius, leftPressure, 0.0f};
    mGroundInteractionB = {rightFootX, radius, rightPressure, 0.0f};
}

void VulkanContext::createInstance()
{
    if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport())
    {
        throw std::runtime_error("[Vulkan] طبقات التحقق غير متوفرة");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "DuckH";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    const auto extensions = getRequiredExtensions();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (ENABLE_VALIDATION_LAYERS)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    }

    if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء VkInstance");
    }
}

void VulkanContext::setupDebugMessenger()
{
    if (!ENABLE_VALIDATION_LAYERS)
    {
        return;
    }

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
                                    const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
                                    void *) -> VkBool32
    {
        std::cerr << "[Vulkan Validation] " << callbackData->pMessage << std::endl;
        return VK_FALSE;
    };

    const auto createDebugMessenger =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT"));
    if (createDebugMessenger == nullptr ||
        createDebugMessenger(mInstance, &createInfo, nullptr, &mDebugMessenger) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] تحذير: فشل إعداد Debug Messenger" << std::endl;
    }
}

void VulkanContext::createSurface(GLFWwindow *window)
{
    if (glfwCreateWindowSurface(mInstance, window, nullptr, &mSurface) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Surface");
    }
}

void VulkanContext::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        throw std::runtime_error("[Vulkan] لا يوجد GPU يدعم Vulkan");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

    int bestScore = 0;
    for (VkPhysicalDevice device : devices)
    {
        const int score = rateDeviceSuitability(device);
        if (score > bestScore)
        {
            bestScore = score;
            mPhysicalDevice = device;
        }
    }

    if (mPhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("[Vulkan] لا يوجد GPU مناسب");
    }
}

void VulkanContext::createLogicalDevice()
{
    const QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    std::set<uint32_t> uniqueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value(),
    };

    const float queuePriority = 1.0f;
    for (uint32_t family : uniqueFamilies)
    {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = family;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;
        queueInfos.push_back(queueInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos = queueInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
    createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

    if (ENABLE_VALIDATION_LAYERS)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    }

    if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Logical Device");
    }

    vkGetDeviceQueue(mDevice, indices.graphicsFamily.value(), 0, &mGraphicsQueue);
    vkGetDeviceQueue(mDevice, indices.presentFamily.value(), 0, &mPresentQueue);
}

SwapchainSupportDetails VulkanContext::querySwapchainSupport(VkPhysicalDevice device) const
{
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mSurface, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR VulkanContext::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available) const
{
    for (const VkSurfaceFormatKHR &format : available)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }
    return available.front();
}

VkPresentModeKHR VulkanContext::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available) const
{
    for (VkPresentModeKHR presentMode : available)
    {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return presentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanContext::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                                           uint32_t width,
                                           uint32_t height) const
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }

    VkExtent2D extent{width, height};
    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return extent;
}

void VulkanContext::createSwapchain()
{
    const SwapchainSupportDetails support = querySwapchainSupport(mPhysicalDevice);
    const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(support.formats);
    const VkPresentModeKHR presentMode = chooseSwapPresentMode(support.presentModes);
    const VkExtent2D extent = chooseSwapExtent(support.capabilities, mWidth, mHeight);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
    {
        imageCount = support.capabilities.maxImageCount;
    }

    const QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice);
    const uint32_t familyIndices[] = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value(),
    };

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = mSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = familyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapchain) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Swapchain");
    }

    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, nullptr);
    mSwapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, mSwapchainImages.data());

    mSwapchainImageFormat = surfaceFormat.format;
    mSwapchainExtent = extent;
    mImagesInFlight.assign(mSwapchainImages.size(), VK_NULL_HANDLE);
}

void VulkanContext::createImageViews()
{
    mSwapchainImageViews.resize(mSwapchainImages.size());

    for (std::size_t i = 0; i < mSwapchainImages.size(); ++i)
    {
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

        if (vkCreateImageView(mDevice, &createInfo, nullptr, &mSwapchainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("[Vulkan] فشل إنشاء Image View");
        }
    }
}

void VulkanContext::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = mSwapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Render Pass");
    }
}

void VulkanContext::createFramebuffers()
{
    mSwapchainFramebuffers.resize(mSwapchainImageViews.size());

    for (std::size_t i = 0; i < mSwapchainImageViews.size(); ++i)
    {
        VkImageView attachments[] = {mSwapchainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = mRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = mSwapchainExtent.width;
        framebufferInfo.height = mSwapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mSwapchainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("[Vulkan] فشل إنشاء Framebuffer");
        }
    }
}

void VulkanContext::createCommandPool()
{
    const QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Command Pool");
    }
}

void VulkanContext::createCommandBuffers()
{
    if (!mCommandBuffers.empty())
    {
        vkFreeCommandBuffers(
            mDevice,
            mCommandPool,
            static_cast<uint32_t>(mCommandBuffers.size()),
            mCommandBuffers.data());
        mCommandBuffers.clear();
    }

    mCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(mCommandBuffers.size());

    if (vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Command Buffers");
    }
}

void VulkanContext::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (mImageAvailableSemaphores.empty())
    {
        mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        for (VkSemaphore &semaphore : mImageAvailableSemaphores)
        {
            if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
            {
                throw std::runtime_error("[Vulkan] فشل إنشاء ImageAvailable Semaphore");
            }
        }
    }

    if (mInFlightFences.empty())
    {
        mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        for (VkFence &fence : mInFlightFences)
        {
            if (vkCreateFence(mDevice, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
            {
                throw std::runtime_error("[Vulkan] فشل إنشاء InFlight Fence");
            }
        }
    }

    for (VkSemaphore semaphore : mRenderFinishedSemaphores)
    {
        vkDestroySemaphore(mDevice, semaphore, nullptr);
    }
    mRenderFinishedSemaphores.clear();
    mRenderFinishedSemaphores.resize(mSwapchainImages.size());

    for (VkSemaphore &semaphore : mRenderFinishedSemaphores)
    {
        if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
        {
            throw std::runtime_error("[Vulkan] فشل إنشاء RenderFinished Semaphore");
        }
    }

    mImagesInFlight.assign(mSwapchainImages.size(), VK_NULL_HANDLE);
}

void VulkanContext::createDescriptorSetLayout()
{
    if (mDescriptorSetLayout != VK_NULL_HANDLE)
    {
        return;
    }

    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding windBinding{};
    windBinding.binding = 1;
    windBinding.descriptorCount = 1;
    windBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    windBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding windMapBinding{};
    windMapBinding.binding = 2;
    windMapBinding.descriptorCount = 1;
    windMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    windMapBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
        samplerBinding,
        windBinding,
        windMapBinding,
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Descriptor Set Layout");
    }
}

void VulkanContext::createWindUniformBuffers()
{
    const VkDeviceSize bufferSize = sizeof(WindUniformData);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            mWindUniformBuffers[i],
            mWindUniformBuffersMemory[i]);

        if (vkMapMemory(mDevice, mWindUniformBuffersMemory[i], 0, bufferSize, 0, &mWindUniformBuffersMapped[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("[Vulkan] فشل ربط ذاكرة wind uniform buffer");
        }

        std::memset(mWindUniformBuffersMapped[i], 0, static_cast<std::size_t>(bufferSize));
    }
}

void VulkanContext::createAtmosphereUniformBuffers()
{
    const VkDeviceSize sunBufferSize = sizeof(SunDiskUniformData);
    const VkDeviceSize godRayBufferSize = sizeof(GodRayUniformData);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        createBuffer(
            sunBufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            mSunUniformBuffers[i],
            mSunUniformBuffersMemory[i]);
        if (vkMapMemory(mDevice, mSunUniformBuffersMemory[i], 0, sunBufferSize, 0, &mSunUniformBuffersMapped[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("[Atmosphere] فشل ربط Sun UBO");
        }
        std::memset(mSunUniformBuffersMapped[i], 0, static_cast<std::size_t>(sunBufferSize));

        createBuffer(
            godRayBufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            mGodRayUniformBuffers[i],
            mGodRayUniformBuffersMemory[i]);
        if (vkMapMemory(mDevice, mGodRayUniformBuffersMemory[i], 0, godRayBufferSize, 0, &mGodRayUniformBuffersMapped[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("[Atmosphere] فشل ربط God Ray UBO");
        }
        std::memset(mGodRayUniformBuffersMapped[i], 0, static_cast<std::size_t>(godRayBufferSize));
    }
}

void VulkanContext::createAtmosphereVertexBuffer()
{
    if (mAtmosphereVertexBuffer != VK_NULL_HANDLE)
    {
        return;
    }

    constexpr std::array<FullscreenVertex, 3> vertices{{
        {-1.0f, -1.0f, 0.0f, 0.0f},
        { 3.0f, -1.0f, 2.0f, 0.0f},
        {-1.0f,  3.0f, 0.0f, 2.0f},
    }};

    const VkDeviceSize bufferSize = sizeof(vertices);
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        mAtmosphereVertexBuffer,
        mAtmosphereVertexBufferMemory);

    void* mappedData = nullptr;
    if (vkMapMemory(mDevice, mAtmosphereVertexBufferMemory, 0, bufferSize, 0, &mappedData) != VK_SUCCESS)
    {
        throw std::runtime_error("[Atmosphere] فشل ربط Vertex Buffer للشاشة الكاملة");
    }

    std::memcpy(mappedData, vertices.data(), static_cast<std::size_t>(bufferSize));
    vkUnmapMemory(mDevice, mAtmosphereVertexBufferMemory);
}

void VulkanContext::createAtmosphereMaskTexture()
{
    if (mSunMaskTexture.image != VK_NULL_HANDLE)
    {
        return;
    }

    constexpr int MASK_WIDTH = 256;
    constexpr int MASK_HEIGHT = 256;
    const std::vector<unsigned char> pixels = generateSunMaskPixels(MASK_WIDTH, MASK_HEIGHT, kDefaultSunDiskUniformData);

    mSunMaskTexture.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    createLayerTextureImageFromPixels(mSunMaskTexture, pixels.data(), MASK_WIDTH, MASK_HEIGHT);
    createLayerTextureImageView(mSunMaskTexture);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;

    if (vkCreateSampler(mDevice, &samplerInfo, nullptr, &mSunMaskTexture.sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("[Atmosphere] فشل إنشاء Sampler لقناع الشمس");
    }
}

void VulkanContext::createAtmosphereDescriptorSets()
{
    const AtmospherePassLayouts& layouts = mAtmosphereRenderer.getLayouts();

    if (mAtmosphereSunDescriptorPool == VK_NULL_HANDLE)
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

        if (vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mAtmosphereSunDescriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("[Atmosphere] فشل إنشاء Descriptor Pool للشمس");
        }

        std::vector<VkDescriptorSetLayout> setLayouts(MAX_FRAMES_IN_FLIGHT, layouts.sunDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = mAtmosphereSunDescriptorPool;
        allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = setLayouts.data();

        if (vkAllocateDescriptorSets(mDevice, &allocInfo, mAtmosphereSunDescriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("[Atmosphere] فشل إنشاء Descriptor Sets للشمس");
        }
    }

    if (mAtmosphereGodRayDescriptorPool == VK_NULL_HANDLE)
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

        if (vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mAtmosphereGodRayDescriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("[Atmosphere] فشل إنشاء Descriptor Pool للأشعة");
        }

        std::vector<VkDescriptorSetLayout> setLayouts(MAX_FRAMES_IN_FLIGHT, layouts.godRayDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = mAtmosphereGodRayDescriptorPool;
        allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = setLayouts.data();

        if (vkAllocateDescriptorSets(mDevice, &allocInfo, mAtmosphereGodRayDescriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("[Atmosphere] فشل إنشاء Descriptor Sets للأشعة");
        }
    }

    for (int frameIndex = 0; frameIndex < MAX_FRAMES_IN_FLIGHT; ++frameIndex)
    {
        VkDescriptorBufferInfo sunBufferInfo{};
        sunBufferInfo.buffer = mSunUniformBuffers[frameIndex];
        sunBufferInfo.offset = 0;
        sunBufferInfo.range = sizeof(SunDiskUniformData);

        VkWriteDescriptorSet sunWrite{};
        sunWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sunWrite.dstSet = mAtmosphereSunDescriptorSets[frameIndex];
        sunWrite.dstBinding = 0;
        sunWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sunWrite.descriptorCount = 1;
        sunWrite.pBufferInfo = &sunBufferInfo;
        vkUpdateDescriptorSets(mDevice, 1, &sunWrite, 0, nullptr);

        VkDescriptorImageInfo maskInfo{};
        maskInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        maskInfo.imageView = mSunMaskTexture.imageView;
        maskInfo.sampler = mSunMaskTexture.sampler;

        VkDescriptorBufferInfo godRayBufferInfo{};
        godRayBufferInfo.buffer = mGodRayUniformBuffers[frameIndex];
        godRayBufferInfo.offset = 0;
        godRayBufferInfo.range = sizeof(GodRayUniformData);

        std::array<VkWriteDescriptorSet, 2> godRayWrites{};
        godRayWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        godRayWrites[0].dstSet = mAtmosphereGodRayDescriptorSets[frameIndex];
        godRayWrites[0].dstBinding = 0;
        godRayWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        godRayWrites[0].descriptorCount = 1;
        godRayWrites[0].pImageInfo = &maskInfo;

        godRayWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        godRayWrites[1].dstSet = mAtmosphereGodRayDescriptorSets[frameIndex];
        godRayWrites[1].dstBinding = 1;
        godRayWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        godRayWrites[1].descriptorCount = 1;
        godRayWrites[1].pBufferInfo = &godRayBufferInfo;

        vkUpdateDescriptorSets(mDevice, static_cast<uint32_t>(godRayWrites.size()), godRayWrites.data(), 0, nullptr);
    }
}

void VulkanContext::createWindTextureResources()
{
    if (mWindTexture.image != VK_NULL_HANDLE)
    {
        return;
    }

    constexpr int WIND_TEX_WIDTH = 128;
    constexpr int WIND_TEX_HEIGHT = 128;
    const std::vector<unsigned char> pixels = generateWindTexturePixels(WIND_TEX_WIDTH, WIND_TEX_HEIGHT);

    mWindTexture.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    createLayerTextureImageFromPixels(mWindTexture, pixels.data(), WIND_TEX_WIDTH, WIND_TEX_HEIGHT);
    createLayerTextureImageView(mWindTexture);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;

    if (vkCreateSampler(mDevice, &samplerInfo, nullptr, &mWindTexture.sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Wind Texture Sampler");
    }
}

void VulkanContext::createAtmospherePipelines()
{
    if (!mAtmosphereRenderer.isInitialized() || mRenderPass == VK_NULL_HANDLE)
    {
        return;
    }

    cleanupAtmospherePipelines();

    const auto vertexShaderCode = readFile("assets/shaders/vert/fullscreen_triangle.vert.spv");
    const auto sunFragmentShaderCode = readFile("assets/shaders/frag/sun_disk.frag.spv");
    const auto godRayFragmentShaderCode = readFile("assets/shaders/frag/god_rays.frag.spv");

    const VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    const VkShaderModule sunFragmentShaderModule = createShaderModule(sunFragmentShaderCode);
    const VkShaderModule godRayFragmentShaderModule = createShaderModule(godRayFragmentShaderCode);

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertexShaderModule;
    shaderStages[0].pName = "main";

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(FullscreenVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(FullscreenVertex, x);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(FullscreenVertex, u);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState additiveBlend =
        AtmosphereRenderer::makeAdditiveBlendAttachment();
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &additiveBlend;

    const AtmospherePassLayouts& layouts = mAtmosphereRenderer.getLayouts();

    auto createPipeline = [&](VkShaderModule fragmentModule,
                              VkPipelineLayout pipelineLayout,
                              VkPipeline& pipelineHandle,
                              const char* errorMessage) {
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragmentModule;
        shaderStages[1].pName = "main";

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = mRenderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipelineHandle) != VK_SUCCESS)
        {
            throw std::runtime_error(errorMessage);
        }
    };

    try
    {
        createPipeline(
            godRayFragmentShaderModule,
            layouts.godRayPipelineLayout,
            mAtmosphereGodRayPipeline,
            "[Atmosphere] فشل إنشاء pipeline للأشعة");
        createPipeline(
            sunFragmentShaderModule,
            layouts.sunPipelineLayout,
            mAtmosphereSunPipeline,
            "[Atmosphere] فشل إنشاء pipeline للشمس");
    }
    catch (...)
    {
        vkDestroyShaderModule(mDevice, godRayFragmentShaderModule, nullptr);
        vkDestroyShaderModule(mDevice, sunFragmentShaderModule, nullptr);
        vkDestroyShaderModule(mDevice, vertexShaderModule, nullptr);
        throw;
    }

    vkDestroyShaderModule(mDevice, godRayFragmentShaderModule, nullptr);
    vkDestroyShaderModule(mDevice, sunFragmentShaderModule, nullptr);
    vkDestroyShaderModule(mDevice, vertexShaderModule, nullptr);
}

void VulkanContext::createSpritePipeline()
{
    if (mRenderPass == VK_NULL_HANDLE || mDescriptorSetLayout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("[Vulkan] لا يمكن إنشاء Sprite Pipeline قبل تجهيز RenderPass و DescriptorSetLayout");
    }

    cleanupSpritePipeline();

    const auto vertexShaderCode = readFile("assets/shaders/vert/sprite.vert.spv");
    const auto fragmentShaderCode = readFile("assets/shaders/frag/sprite.frag.spv");

    const VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    const VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertexShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragmentShaderModule;
    shaderStages[1].pName = "main";

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(QuadVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 7> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(QuadVertex, x);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(QuadVertex, u);
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(QuadVertex, alpha);
    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(QuadVertex, windWeight);
    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(QuadVertex, windPhase);
    attributeDescriptions[5].binding = 0;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32_SFLOAT;
    attributeDescriptions[5].offset = offsetof(QuadVertex, windResponse);
    attributeDescriptions[6].binding = 0;
    attributeDescriptions[6].location = 6;
    attributeDescriptions[6].format = VK_FORMAT_R32_SFLOAT;
    attributeDescriptions[6].offset = offsetof(QuadVertex, materialType);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{};
    viewport.width = static_cast<float>(mSwapchainExtent.width);
    viewport.height = static_cast<float>(mSwapchainExtent.height);
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{{0, 0}, mSwapchainExtent};

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &mDescriptorSetLayout;

    if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mSpritePipelineLayout) != VK_SUCCESS)
    {
        vkDestroyShaderModule(mDevice, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(mDevice, vertexShaderModule, nullptr);
        throw std::runtime_error("[Vulkan] فشل إنشاء Pipeline Layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = mSpritePipelineLayout;
    pipelineInfo.renderPass = mRenderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mSpritePipeline) != VK_SUCCESS)
    {
        vkDestroyShaderModule(mDevice, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(mDevice, vertexShaderModule, nullptr);
        throw std::runtime_error("[Vulkan] فشل إنشاء Graphics Pipeline");
    }

    vkDestroyShaderModule(mDevice, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(mDevice, vertexShaderModule, nullptr);
}

void VulkanContext::createLayerTextureImageFromPixels(SpriteLayerResources &layer,
                                                      const unsigned char *pixels,
                                                      int texW,
                                                      int texH)
{
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(texW) * static_cast<VkDeviceSize>(texH) * 4;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    createBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    void *mappedData = nullptr;
    if (vkMapMemory(mDevice, stagingBufferMemory, 0, imageSize, 0, &mappedData) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل ربط staging buffer");
    }
    std::memcpy(mappedData, pixels, static_cast<std::size_t>(imageSize));
    vkUnmapMemory(mDevice, stagingBufferMemory);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(texW);
    imageInfo.extent.height = static_cast<uint32_t>(texH);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = layer.imageFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(mDevice, &imageInfo, nullptr, &layer.image) != VK_SUCCESS)
    {
        vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
        vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
        throw std::runtime_error("[Vulkan] فشل إنشاء صورة النسيج");
    }

    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(mDevice, layer.image, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &layer.imageMemory) != VK_SUCCESS)
    {
        vkDestroyImage(mDevice, layer.image, nullptr);
        layer.image = VK_NULL_HANDLE;
        vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
        vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
        throw std::runtime_error("[Vulkan] فشل حجز ذاكرة صورة النسيج");
    }

    if (vkBindImageMemory(mDevice, layer.image, layer.imageMemory, 0) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل ربط ذاكرة صورة النسيج");
    }

    transitionImageLayout(layer.image, layer.imageFormat,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, layer.image, static_cast<uint32_t>(texW), static_cast<uint32_t>(texH));
    transitionImageLayout(layer.image, layer.imageFormat,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
    vkFreeMemory(mDevice, stagingBufferMemory, nullptr);

    std::cout << "[Vulkan] تم تحميل النسيج: " << texW << "x" << texH << std::endl;
}

void VulkanContext::createLayerTextureImageView(SpriteLayerResources &layer)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = layer.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = layer.imageFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(mDevice, &viewInfo, nullptr, &layer.imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Image View للنسيج");
    }
}

void VulkanContext::createLayerTextureSampler(SpriteLayerResources &layer)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
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
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;

    if (vkCreateSampler(mDevice, &samplerInfo, nullptr, &layer.sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Sampler");
    }
}

void VulkanContext::createLayerDescriptors(SpriteLayerResources &layer)
{
    if (mWindTexture.imageView == VK_NULL_HANDLE || mWindTexture.sampler == VK_NULL_HANDLE)
    {
        throw std::runtime_error("[Vulkan] لا يمكن إنشاء descriptor للطبقة قبل تهيئة Wind Texture");
    }

    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT * 2;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &layer.descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Descriptor Pool");
    }

    layer.descriptorSets.assign(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, mDescriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = layer.descriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(mDevice, &allocInfo, layer.descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Descriptor Set");
    }

    for (int frameIndex = 0; frameIndex < MAX_FRAMES_IN_FLIGHT; ++frameIndex)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = layer.imageView;
        imageInfo.sampler = layer.sampler;

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = mWindUniformBuffers[frameIndex];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(WindUniformData);

        VkDescriptorImageInfo windImageInfo{};
        windImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        windImageInfo.imageView = mWindTexture.imageView;
        windImageInfo.sampler = mWindTexture.sampler;

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = layer.descriptorSets[frameIndex];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].pImageInfo = &imageInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = layer.descriptorSets[frameIndex];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].pBufferInfo = &bufferInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = layer.descriptorSets[frameIndex];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].pImageInfo = &windImageInfo;

        vkUpdateDescriptorSets(
            mDevice,
            static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(),
            0,
            nullptr);
    }
}

void VulkanContext::createLayerVertexBuffer(SpriteLayerResources &layer)
{
    const VkDeviceSize bufferSize =
        static_cast<VkDeviceSize>(sizeof(QuadVertex)) * QUAD_VERTEX_COUNT * layer.maxQuads;

    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        layer.vertexBuffer,
        layer.vertexBufferMemory);

    if (vkMapMemory(mDevice, layer.vertexBufferMemory, 0, bufferSize, 0, &layer.vertexBufferMapped) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل ربط ذاكرة vertex buffer");
    }

    std::memset(layer.vertexBufferMapped, 0, static_cast<std::size_t>(bufferSize));
    layer.vertexCount = 0;
}

void VulkanContext::destroyLayerResources(SpriteLayerResources &layer)
{
    if (layer.vertexBufferMapped != nullptr && layer.vertexBufferMemory != VK_NULL_HANDLE)
    {
        vkUnmapMemory(mDevice, layer.vertexBufferMemory);
        layer.vertexBufferMapped = nullptr;
    }

    if (layer.vertexBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(mDevice, layer.vertexBuffer, nullptr);
        layer.vertexBuffer = VK_NULL_HANDLE;
    }
    if (layer.vertexBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(mDevice, layer.vertexBufferMemory, nullptr);
        layer.vertexBufferMemory = VK_NULL_HANDLE;
    }
    if (layer.descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(mDevice, layer.descriptorPool, nullptr);
        layer.descriptorPool = VK_NULL_HANDLE;
    }
    layer.descriptorSets.clear();
    if (layer.sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(mDevice, layer.sampler, nullptr);
        layer.sampler = VK_NULL_HANDLE;
    }
    if (layer.imageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(mDevice, layer.imageView, nullptr);
        layer.imageView = VK_NULL_HANDLE;
    }
    if (layer.image != VK_NULL_HANDLE)
    {
        vkDestroyImage(mDevice, layer.image, nullptr);
        layer.image = VK_NULL_HANDLE;
    }
    if (layer.imageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(mDevice, layer.imageMemory, nullptr);
        layer.imageMemory = VK_NULL_HANDLE;
    }
    layer.vertexCount = 0;
    layer.maxQuads = 0;
}

void VulkanContext::destroyAtmosphereUniformBuffers()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (mSunUniformBuffersMapped[i] != nullptr && mSunUniformBuffersMemory[i] != VK_NULL_HANDLE)
        {
            vkUnmapMemory(mDevice, mSunUniformBuffersMemory[i]);
            mSunUniformBuffersMapped[i] = nullptr;
        }
        if (mSunUniformBuffers[i] != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(mDevice, mSunUniformBuffers[i], nullptr);
            mSunUniformBuffers[i] = VK_NULL_HANDLE;
        }
        if (mSunUniformBuffersMemory[i] != VK_NULL_HANDLE)
        {
            vkFreeMemory(mDevice, mSunUniformBuffersMemory[i], nullptr);
            mSunUniformBuffersMemory[i] = VK_NULL_HANDLE;
        }

        if (mGodRayUniformBuffersMapped[i] != nullptr && mGodRayUniformBuffersMemory[i] != VK_NULL_HANDLE)
        {
            vkUnmapMemory(mDevice, mGodRayUniformBuffersMemory[i]);
            mGodRayUniformBuffersMapped[i] = nullptr;
        }
        if (mGodRayUniformBuffers[i] != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(mDevice, mGodRayUniformBuffers[i], nullptr);
            mGodRayUniformBuffers[i] = VK_NULL_HANDLE;
        }
        if (mGodRayUniformBuffersMemory[i] != VK_NULL_HANDLE)
        {
            vkFreeMemory(mDevice, mGodRayUniformBuffersMemory[i], nullptr);
            mGodRayUniformBuffersMemory[i] = VK_NULL_HANDLE;
        }

        mAtmosphereSunDescriptorSets[i] = VK_NULL_HANDLE;
        mAtmosphereGodRayDescriptorSets[i] = VK_NULL_HANDLE;
    }
}

void VulkanContext::destroyAtmosphereVertexBuffer()
{
    if (mAtmosphereVertexBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(mDevice, mAtmosphereVertexBuffer, nullptr);
        mAtmosphereVertexBuffer = VK_NULL_HANDLE;
    }
    if (mAtmosphereVertexBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(mDevice, mAtmosphereVertexBufferMemory, nullptr);
        mAtmosphereVertexBufferMemory = VK_NULL_HANDLE;
    }
}

void VulkanContext::destroyAtmosphereDescriptorSets()
{
    if (mAtmosphereSunDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(mDevice, mAtmosphereSunDescriptorPool, nullptr);
        mAtmosphereSunDescriptorPool = VK_NULL_HANDLE;
    }
    if (mAtmosphereGodRayDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(mDevice, mAtmosphereGodRayDescriptorPool, nullptr);
        mAtmosphereGodRayDescriptorPool = VK_NULL_HANDLE;
    }
    mAtmosphereSunDescriptorSets.fill(VK_NULL_HANDLE);
    mAtmosphereGodRayDescriptorSets.fill(VK_NULL_HANDLE);
}

void VulkanContext::destroyAtmosphereMaskTexture()
{
    destroyLayerResources(mSunMaskTexture);
}

void VulkanContext::updateWindUniformBuffer(uint32_t frameIndex)
{
    if (frameIndex >= static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) ||
        mWindUniformBuffersMapped[frameIndex] == nullptr)
    {
        return;
    }

    WindUniformData data;
    data.motion[0] = static_cast<float>(glfwGetTime());
    data.interactionA = mGroundInteractionA;
    data.interactionB = mGroundInteractionB;
    std::memcpy(mWindUniformBuffersMapped[frameIndex], &data, sizeof(data));
}

void VulkanContext::destroyWindUniformBuffers()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (mWindUniformBuffersMapped[i] != nullptr && mWindUniformBuffersMemory[i] != VK_NULL_HANDLE)
        {
            vkUnmapMemory(mDevice, mWindUniformBuffersMemory[i]);
            mWindUniformBuffersMapped[i] = nullptr;
        }

        if (mWindUniformBuffers[i] != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(mDevice, mWindUniformBuffers[i], nullptr);
            mWindUniformBuffers[i] = VK_NULL_HANDLE;
        }

        if (mWindUniformBuffersMemory[i] != VK_NULL_HANDLE)
        {
            vkFreeMemory(mDevice, mWindUniformBuffersMemory[i], nullptr);
            mWindUniformBuffersMemory[i] = VK_NULL_HANDLE;
        }
    }
}

void VulkanContext::destroyWindTextureResources()
{
    destroyLayerResources(mWindTexture);
}

std::vector<const char *> VulkanContext::getRequiredExtensions() const
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (ENABLE_VALIDATION_LAYERS)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

bool VulkanContext::checkValidationLayerSupport() const
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : VALIDATION_LAYERS)
    {
        bool found = false;
        for (const VkLayerProperties &layerProperties : availableLayers)
        {
            if (std::strcmp(layerName, layerProperties.layerName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            return false;
        }
    }
    return true;
}

bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice device) const
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    for (const char *requiredExtension : DEVICE_EXTENSIONS)
    {
        bool found = false;
        for (const VkExtensionProperties &extension : availableExtensions)
        {
            if (std::strcmp(requiredExtension, extension.extensionName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            return false;
        }
    }
    return true;
}

int VulkanContext::rateDeviceSuitability(VkPhysicalDevice device) const
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device, &properties);

    int score = 0;
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }

    score += static_cast<int>(properties.limits.maxImageDimension2D);

    if (!findQueueFamilies(device).isComplete())
    {
        return 0;
    }
    if (!checkDeviceExtensionSupport(device))
    {
        return 0;
    }

    const SwapchainSupportDetails support = querySwapchainSupport(device);
    if (support.formats.empty() || support.presentModes.empty())
    {
        return 0;
    }

    return score;
}

QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }
    }

    return indices;
}

VkShaderModule VulkanContext::createShaderModule(const std::vector<char> &code) const
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(mDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Shader Module");
    }
    return shaderModule;
}

std::vector<char> VulkanContext::readFile(const std::string &path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("[Vulkan] فشل قراءة ملف: " + path);
    }

    const std::size_t fileSize = static_cast<std::size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    return buffer;
}

uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1u << i)) &&
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("[Vulkan] لم يُعثر على نوع ذاكرة مناسب");
}

VulkanContext::SpriteLayerResources &VulkanContext::getLayer(LayerId layerId)
{
    if (layerId >= mSpriteLayers.size())
    {
        throw std::runtime_error("[Vulkan] معرف الطبقة غير صالح");
    }
    return mSpriteLayers[layerId];
}

const VulkanContext::SpriteLayerResources &VulkanContext::getLayer(LayerId layerId) const
{
    if (layerId >= mSpriteLayers.size())
    {
        throw std::runtime_error("[Vulkan] معرف الطبقة غير صالح");
    }
    return mSpriteLayers[layerId];
}

void VulkanContext::createBuffer(VkDeviceSize size,
                                 VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags properties,
                                 VkBuffer &buffer,
                                 VkDeviceMemory &memory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("[Vulkan] فشل إنشاء Buffer");
    }

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(mDevice, buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &memory) != VK_SUCCESS)
    {
        vkDestroyBuffer(mDevice, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        throw std::runtime_error("[Vulkan] فشل حجز ذاكرة Buffer");
    }

    if (vkBindBufferMemory(mDevice, buffer, memory, 0) != VK_SUCCESS)
    {
        vkDestroyBuffer(mDevice, buffer, nullptr);
        vkFreeMemory(mDevice, memory, nullptr);
        buffer = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
        throw std::runtime_error("[Vulkan] فشل ربط ذاكرة Buffer");
    }
}

void VulkanContext::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = mCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mGraphicsQueue);
    vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer);
}

void VulkanContext::transitionImageLayout(VkImage image,
                                          VkFormat /*format*/,
                                          VkImageLayout oldLayout,
                                          VkImageLayout newLayout)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = mCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = 0;
    VkPipelineStageFlags destinationStage = 0;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::runtime_error("[Vulkan] انتقال تخطيط غير مدعوم");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage,
        destinationStage,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mGraphicsQueue);
    vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer);
}

void VulkanContext::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t texWidth, uint32_t texHeight)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = mCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {texWidth, texHeight, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mGraphicsQueue);
    vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer);
}

void VulkanContext::cleanupSpritePipeline()
{
    if (mSpritePipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(mDevice, mSpritePipeline, nullptr);
        mSpritePipeline = VK_NULL_HANDLE;
    }

    if (mSpritePipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(mDevice, mSpritePipelineLayout, nullptr);
        mSpritePipelineLayout = VK_NULL_HANDLE;
    }
}

void VulkanContext::cleanupAtmospherePipelines()
{
    if (mAtmosphereSunPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(mDevice, mAtmosphereSunPipeline, nullptr);
        mAtmosphereSunPipeline = VK_NULL_HANDLE;
    }
    if (mAtmosphereGodRayPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(mDevice, mAtmosphereGodRayPipeline, nullptr);
        mAtmosphereGodRayPipeline = VK_NULL_HANDLE;
    }
}

void VulkanContext::waitForValidFramebufferSize()
{
    if (mWindowHandle == nullptr)
    {
        return;
    }

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(mWindowHandle, &framebufferWidth, &framebufferHeight);

    while (framebufferWidth == 0 || framebufferHeight == 0)
    {
        if (glfwWindowShouldClose(mWindowHandle))
        {
            mWidth = 0;
            mHeight = 0;
            return;
        }

        glfwWaitEvents();
        glfwGetFramebufferSize(mWindowHandle, &framebufferWidth, &framebufferHeight);
    }

    mWidth = static_cast<uint32_t>(framebufferWidth);
    mHeight = static_cast<uint32_t>(framebufferHeight);
}

bool VulkanContext::hasSpriteResources() const
{
    return !mSpriteLayers.empty();
}

} // namespace gfx
