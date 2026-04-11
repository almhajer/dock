#include "AtmosphereRenderer.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace gfx
{
namespace
{

VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutBinding* bindings,
                                                uint32_t bindingCount, const char* errorMessage)
{
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindingCount;
    layoutInfo.pBindings = bindings;

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
    {
        throw std::runtime_error(errorMessage);
    }
    return layout;
}

VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
                                      const char* errorMessage)
{
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout;

    VkPipelineLayout layout = VK_NULL_HANDLE;
    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
    {
        throw std::runtime_error(errorMessage);
    }
    return layout;
}

} // namespace

void AtmosphereRenderer::init(VkDevice device, VkExtent2D sceneExtent)
{
    if (mInitialized)
    {
        onResize(sceneExtent);
        return;
    }

    mDevice = device;
    mSceneExtent = sceneExtent;

    createDescriptorSetLayouts();
    createPipelineLayouts();
    mInitialized = true;
}

void AtmosphereRenderer::cleanup()
{
    if (!mInitialized)
    {
        return;
    }

    destroyPipelineLayouts();
    destroyDescriptorSetLayouts();

    mDevice = VK_NULL_HANDLE;
    mSceneExtent = {};
    mInitialized = false;
}

void AtmosphereRenderer::onResize(VkExtent2D sceneExtent)
{
    mSceneExtent = sceneExtent;
}

bool AtmosphereRenderer::isInitialized() const
{
    return mInitialized;
}

const AtmospherePassLayouts& AtmosphereRenderer::getLayouts() const
{
    return mLayouts;
}

VkPipelineColorBlendAttachmentState AtmosphereRenderer::makeAdditiveBlendAttachment()
{
    VkPipelineColorBlendAttachmentState blend{};
    blend.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend.blendEnable = VK_TRUE;
    blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blend.colorBlendOp = VK_BLEND_OP_ADD;
    blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend.alphaBlendOp = VK_BLEND_OP_ADD;
    return blend;
}

void AtmosphereRenderer::createDescriptorSetLayouts()
{
    std::array<VkDescriptorSetLayoutBinding, 2> sunBindings{};
    sunBindings[0].binding = 0;
    sunBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sunBindings[0].descriptorCount = 1;
    sunBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    sunBindings[1].binding = 1;
    sunBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sunBindings[1].descriptorCount = 1;
    sunBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    mLayouts.sunDescriptorSetLayout =
        createDescriptorSetLayout(mDevice, sunBindings.data(), static_cast<uint32_t>(sunBindings.size()),
                                  "[Atmosphere] فشل إنشاء descriptor layout للشمس");

    std::array<VkDescriptorSetLayoutBinding, 3> godRayBindings{};
    godRayBindings[0].binding = 0;
    godRayBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    godRayBindings[0].descriptorCount = 1;
    godRayBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    godRayBindings[1].binding = 1;
    godRayBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    godRayBindings[1].descriptorCount = 1;
    godRayBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    godRayBindings[2].binding = 2;
    godRayBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    godRayBindings[2].descriptorCount = 1;
    godRayBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    mLayouts.godRayDescriptorSetLayout =
        createDescriptorSetLayout(mDevice, godRayBindings.data(), static_cast<uint32_t>(godRayBindings.size()),
                                  "[Atmosphere] فشل إنشاء descriptor layout للأشعة الحجمية");
}

void AtmosphereRenderer::createPipelineLayouts()
{
    mLayouts.sunPipelineLayout =
        createPipelineLayout(mDevice, mLayouts.sunDescriptorSetLayout, "[Atmosphere] فشل إنشاء pipeline layout للشمس");

    mLayouts.godRayPipelineLayout = createPipelineLayout(mDevice, mLayouts.godRayDescriptorSetLayout,
                                                         "[Atmosphere] فشل إنشاء pipeline layout للأشعة الحجمية");
}

void AtmosphereRenderer::destroyPipelineLayouts()
{
    if (mLayouts.godRayPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(mDevice, mLayouts.godRayPipelineLayout, nullptr);
        mLayouts.godRayPipelineLayout = VK_NULL_HANDLE;
    }
    if (mLayouts.sunPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(mDevice, mLayouts.sunPipelineLayout, nullptr);
        mLayouts.sunPipelineLayout = VK_NULL_HANDLE;
    }
}

void AtmosphereRenderer::destroyDescriptorSetLayouts()
{
    if (mLayouts.godRayDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(mDevice, mLayouts.godRayDescriptorSetLayout, nullptr);
        mLayouts.godRayDescriptorSetLayout = VK_NULL_HANDLE;
    }
    if (mLayouts.sunDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(mDevice, mLayouts.sunDescriptorSetLayout, nullptr);
        mLayouts.sunDescriptorSetLayout = VK_NULL_HANDLE;
    }
}

} // namespace gfx
