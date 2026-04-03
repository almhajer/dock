#pragma once

#include <array>
#include <cstddef>
#include <vulkan/vulkan.h>

namespace gfx
{

inline constexpr std::size_t ATMOSPHERE_MAX_CLOUD_OCCLUDERS = 16;

struct SunDiskUniformData
{
    alignas(16) std::array<float, 4> sunDisk = {0.82f, 0.18f, 0.06f, 0.18f};          // xy = screen pos, z = disk radius, w = corona radius
    alignas(16) std::array<float, 4> sunColorIntensity = {1.0f, 0.96f, 0.84f, 18.0f}; // rgb = color, a = hdr intensity
    alignas(16) std::array<float, 4> appearance = {0.58f, 12.0f, 0.02f, 0.0f};        // x = limb darkening, y = corona falloff, z = softness, w = mask only
};

struct GodRayUniformData
{
    alignas(16) std::array<float, 4> sunPosDensityWeight = {0.82f, 0.18f, 0.92f, 0.055f};
    alignas(16) std::array<float, 4> decayExposureJitterSamples = {0.96f, 1.15f, 0.35f, 64.0f};
};

struct AtmosphereCloudOcclusionData
{
    alignas(16) std::array<float, 4> params = {0.0f, 0.92f, 0.0f, 0.0f}; // x = count, y = شدة الحجب, z/w = محجوز
    alignas(16) std::array<std::array<float, 4>, ATMOSPHERE_MAX_CLOUD_OCCLUDERS> cloudRects{}; // xy = center uv, zw = half size uv
    alignas(16) std::array<std::array<float, 4>, ATMOSPHERE_MAX_CLOUD_OCCLUDERS> cloudShape{}; // x = alpha, y = phase, z = softness, w = parallax
};

struct FullscreenVertex
{
    float x = 0.0f;
    float y = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
};

struct AtmospherePassLayouts
{
    VkDescriptorSetLayout sunDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout godRayDescriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout sunPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout godRayPipelineLayout = VK_NULL_HANDLE;
};

class AtmosphereRenderer
{
public:
    AtmosphereRenderer() = default;
    ~AtmosphereRenderer() = default;

    AtmosphereRenderer(const AtmosphereRenderer&) = delete;
    AtmosphereRenderer& operator=(const AtmosphereRenderer&) = delete;

    void init(VkDevice device, VkExtent2D sceneExtent);
    void cleanup();
    void onResize(VkExtent2D sceneExtent);

    [[nodiscard]] bool isInitialized() const;
    [[nodiscard]] const AtmospherePassLayouts& getLayouts() const;

    [[nodiscard]] static VkPipelineColorBlendAttachmentState makeAdditiveBlendAttachment();

private:
    void createDescriptorSetLayouts();
    void createPipelineLayouts();
    void destroyPipelineLayouts();
    void destroyDescriptorSetLayouts();

    VkDevice mDevice = VK_NULL_HANDLE;
    VkExtent2D mSceneExtent{};
    AtmospherePassLayouts mLayouts{};
    bool mInitialized = false;
};

} // namespace gfx
