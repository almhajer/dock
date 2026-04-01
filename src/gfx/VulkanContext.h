#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <array>
#include <optional>
#include <functional>

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

        /// تحميل نسيج سبرايت من ملف PNG
        void loadTexture(const std::string &path);

        /// تحميل نسيج + تمرير بيانات البكسلات لـ callback قبل تحريرها
        using PixelCallback = std::function<void(int w, int h, const unsigned char* pixels)>;
        void loadTextureWithCallback(const std::string &path, PixelCallback cb);

        /// تحديث رؤوس الـ quad مباشرة من حدود الشاشة و UV
        void updateSpriteQuad(float u0, float u1, float v0, float v1,
                              float x0, float x1, float y0, float y1);

    private:
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
        void createTextureImage(const std::string &path);
        void createTextureImageFromPixels(const unsigned char *pixels, int texW, int texH);
        void createTextureImageView();
        void createTextureSampler();
        void createSpriteDescriptors();
        void createSpritePipeline();
        void createSpriteVertexBuffer();

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
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
                          VkBuffer &buffer, VkDeviceMemory &memory);
        void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t texWidth, uint32_t texHeight);
        void cleanupSpritePipeline();
        void cleanupSpriteDescriptors();
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
        VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;

        // نسيج السبرايت
        VkImage mTextureImage = VK_NULL_HANDLE;
        VkDeviceMemory mTextureImageMemory = VK_NULL_HANDLE;
        VkImageView mTextureImageView = VK_NULL_HANDLE;
        VkSampler mTextureSampler = VK_NULL_HANDLE;
        uint32_t mMipLevels = 1;

        // رؤوس السبرايت (quad مع UV)
        VkBuffer mSpriteVertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory mSpriteVertexBufferMemory = VK_NULL_HANDLE;

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

        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

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
