#pragma once

#include "RenderTypes.h"

#include <array>
#include <cstddef>
#include <vulkan/vulkan.h>
#include <functional>
#include <optional>
#include <string>
#include <vector>

struct GLFWwindow;

namespace gfx
{

    /// تفاصيل عائلة الطوابير (Graphics, Present)
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        [[nodiscard]] bool isComplete() const
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    /// تفاصيل قدرات السوابتشين
    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    /// سياق Vulkan الكامل - يدير Instance, Device, Swapchain, Pipeline, RenderPass
    class VulkanContext
    {
    public:
        using LayerId = std::size_t;
        static constexpr LayerId INVALID_LAYER_ID = static_cast<LayerId>(-1);
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        VulkanContext() = default;
        ~VulkanContext();

        // منع النسخ لأن هذا مورد Vulkan وحيد
        VulkanContext(const VulkanContext &) = delete;
        VulkanContext &operator=(const VulkanContext &) = delete;

        /// تهيئة Vulkan بالكامل (instance → device → swapchain → pipeline)
        void init(GLFWwindow *window, uint32_t width, uint32_t height);

        /// تنظيف كل موارد Vulkan
        void cleanup();

        /// إعادة إنشاء السوابتشين عند تغيير حجم النافذة
        void onResize(uint32_t newWidth, uint32_t newHeight);

        /// رسم إطار واحد (acquire → submit → present)
        void drawFrame();

        /// انتظار انتهاء جميع أوامر GPU
        void waitIdle();

        /// هل التهيئة تمت بنجاح؟
        [[nodiscard]] bool isInitialized() const;

        using PixelCallback = std::function<void(int w, int h, const unsigned char* pixels)>;

        /// إنشاء طبقة رسم textured quads من ملف صورة
        [[nodiscard]] LayerId createTexturedLayer(const std::string &path, std::size_t maxQuads);

        /// إنشاء طبقة رسم textured quads من بكسلات جاهزة في الذاكرة
        [[nodiscard]] LayerId createTexturedLayerFromPixels(const unsigned char *pixels,
                                                            int texW,
                                                            int texH,
                                                            std::size_t maxQuads);

        /// إنشاء طبقة رسم textured quads مع callback لمعالجة البكسلات قبل رفعها إلى GPU
        [[nodiscard]] LayerId createTexturedLayerWithCallback(const std::string &path,
                                                              std::size_t maxQuads,
                                                              PixelCallback cb);

        /// تحديث quads الخاصة بطبقة معينة
        void updateTexturedLayer(LayerId layerId, const std::vector<TexturedQuad> &quads);

        /// تحديث موضع وتأثير القدمين على شريط العشب/التربة
        void setGroundInteraction(float leftFootX,
                                  float rightFootX,
                                  float radius,
                                  float leftPressure,
                                  float rightPressure);

    private:
        struct SpriteLayerResources
        {
            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory imageMemory = VK_NULL_HANDLE;
            VkImageView imageView = VK_NULL_HANDLE;
            VkSampler sampler = VK_NULL_HANDLE;
            VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
            std::vector<VkDescriptorSet> descriptorSets;
            VkBuffer vertexBuffer = VK_NULL_HANDLE;
            VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
            void *vertexBufferMapped = nullptr;
            std::size_t maxQuads = 0;
            uint32_t vertexCount = 0;
        };

        // ─── إنشاء المكونات ─────────────────────────────────────────
        void createInstance();
        void setupDebugMessenger();
        void createSurface(GLFWwindow *window);
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSwapchain();
        void createImageViews();
        void createRenderPass();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
        void createDescriptorSetLayout();
        void createSpritePipeline();
        void createWindUniformBuffers();
        void createLayerTextureImageFromPixels(SpriteLayerResources &layer,
                                               const unsigned char *pixels,
                                               int texW,
                                               int texH);
        void createLayerTextureImageView(SpriteLayerResources &layer);
        void createLayerTextureSampler(SpriteLayerResources &layer);
        void createLayerDescriptors(SpriteLayerResources &layer);
        void createLayerVertexBuffer(SpriteLayerResources &layer);
        void updateWindUniformBuffer(uint32_t frameIndex);
        [[nodiscard]] LayerId createLayerFromPixels(const unsigned char *pixels,
                                                    int texW,
                                                    int texH,
                                                    std::size_t maxQuads);
        void destroyLayerResources(SpriteLayerResources &layer);
        void destroyWindUniformBuffers();

        // ─── أدوات مساعدة ───────────────────────────────────────────
        [[nodiscard]] std::vector<const char *> getRequiredExtensions() const;
        [[nodiscard]] bool checkValidationLayerSupport() const;
        [[nodiscard]] bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
        [[nodiscard]] int rateDeviceSuitability(VkPhysicalDevice device) const;
        [[nodiscard]] QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
        [[nodiscard]] SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device) const;
        [[nodiscard]] VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available) const;
        [[nodiscard]] VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available) const;
        [[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height) const;
        [[nodiscard]] VkShaderModule createShaderModule(const std::vector<char> &code) const;
        [[nodiscard]] static std::vector<char> readFile(const std::string &path);
        [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) const;
        [[nodiscard]] SpriteLayerResources &getLayer(LayerId layerId);
        [[nodiscard]] const SpriteLayerResources &getLayer(LayerId layerId) const;
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
                          VkBuffer &buffer, VkDeviceMemory &memory);
        void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t texWidth, uint32_t texHeight);
        void cleanupSpritePipeline();
        void waitForValidFramebufferSize();
        [[nodiscard]] bool hasSpriteResources() const;

        /// تنظيف السوابتشين فقط (لإعادة إنشائها)
        void cleanupSwapchain();
        void recreateSwapchain();

        // ─── أعضاء Vulkan ────────────────────────────────────────────
        bool mInitialized = false;
        GLFWwindow *mWindowHandle = nullptr;

        VkInstance mInstance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR mSurface = VK_NULL_HANDLE;

        VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
        VkDevice mDevice = VK_NULL_HANDLE;

        VkQueue mGraphicsQueue = VK_NULL_HANDLE;
        VkQueue mPresentQueue = VK_NULL_HANDLE;

        VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
        std::vector<VkImage> mSwapchainImages;
        VkFormat mSwapchainImageFormat;
        VkExtent2D mSwapchainExtent;
        std::vector<VkImageView> mSwapchainImageViews;
        std::vector<VkFramebuffer> mSwapchainFramebuffers;

        VkRenderPass mRenderPass = VK_NULL_HANDLE;

        // سبرايت
        VkPipelineLayout mSpritePipelineLayout = VK_NULL_HANDLE;
        VkPipeline mSpritePipeline = VK_NULL_HANDLE;
        VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
        std::vector<SpriteLayerResources> mSpriteLayers;
        std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> mWindUniformBuffers{};
        std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> mWindUniformBuffersMemory{};
        std::array<void *, MAX_FRAMES_IN_FLIGHT> mWindUniformBuffersMapped{};
        std::array<float, 4> mGroundInteractionA{0.0f, 0.0f, 0.0f, 0.0f};
        std::array<float, 4> mGroundInteractionB{0.0f, 0.0f, 0.0f, 0.0f};

        VkCommandPool mCommandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> mCommandBuffers;

        // acquire/fence لكل إطار متوازٍ، و render-finished لكل صورة swapchain
        std::vector<VkSemaphore> mImageAvailableSemaphores;
        std::vector<VkSemaphore> mRenderFinishedSemaphores;
        std::vector<VkFence> mInFlightFences;
        std::vector<VkFence> mImagesInFlight;

        uint32_t mWidth = 0;
        uint32_t mHeight = 0;
        uint32_t mCurrentFrame = 0;
        bool mSwapchainDirty = false;

        // طبقات التحقق (Debug فقط)
        static constexpr bool ENABLE_VALIDATION_LAYERS =
#ifdef NDEBUG
            false;
#else
            true;
#endif

        static const std::vector<const char *> VALIDATION_LAYERS;
        static const std::vector<const char *> DEVICE_EXTENSIONS;
    };

} // namespace gfx
