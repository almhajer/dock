#pragma once

#include <array>
#include <cstddef>
#include <vulkan/vulkan.h>

namespace gfx
{

/*
 @brief العدد الأقصى للغيوم التي تحجب ضوء الشمس داخل مؤثر الجو.
 */
inline constexpr std::size_t ATMOSPHERE_MAX_CLOUD_OCCLUDERS = 16;

/*
 @brief بيانات uniform الخاصة بقرص الشمس والهالة.
 */
struct SunDiskUniformData
{
    /*
     @brief موضع الشمس ونصف قطر القرص والهالة.
     */
    alignas(16) std::array<float, 4> sunDisk = {0.82f, 0.18f, 0.06f,
                                                0.18f}; // xy = screen pos, z = disk radius, w = corona radius
    /*
     @brief لون الشمس وشدة الإضاءة.
     */
    alignas(16) std::array<float, 4> sunColorIntensity = {1.0f, 0.96f, 0.84f, 18.0f}; // rgb = color, a = hdr intensity
    /*
     @brief شكل الحافة والهالة والنعومة.
     */
    alignas(16) std::array<float, 4> appearance = {
        0.58f, 12.0f, 0.02f, 0.0f}; // x = limb darkening, y = corona falloff, z = softness, w = mask only
};

/*
 @brief بيانات uniform الخاصة بأشعة الشمس.
 */
struct GodRayUniformData
{
    /*
     @brief موضع الشمس وكثافة الأشعة ووزنها.
     */
    alignas(16) std::array<float, 4> sunPosDensityWeight = {0.82f, 0.18f, 0.92f, 0.055f};
    /*
     @brief معامل الاضمحلال والتعريض والارتعاش وعدد العينات.
     */
    alignas(16) std::array<float, 4> decayExposureJitterSamples = {0.96f, 1.15f, 0.35f, 64.0f};
};

/*
 @brief بيانات حجب الشمس بواسطة الغيوم.
 */
struct AtmosphereCloudOcclusionData
{
    /*
     @brief عدد السحب وشدة الحجب وبعض الحقول المحجوزة.
     */
    alignas(16) std::array<float, 4> params = {0.0f, 0.92f, 0.0f, 0.0f}; // x = count, y = شدة الحجب, z/w = محجوز
    /*
     @brief مستطيلات تموضع الغيوم على الشاشة.
     */
    alignas(16) std::array<std::array<float, 4>,
                           ATMOSPHERE_MAX_CLOUD_OCCLUDERS> cloudRects{}; // xy = center uv, zw = half size uv
    /*
     @brief شكل كل غيمة من حيث الشفافية والطور والنعومة.
     */
    alignas(
        16) std::array<std::array<float, 4>,
                       ATMOSPHERE_MAX_CLOUD_OCCLUDERS> cloudShape{}; // x = alpha, y = phase, z = softness, w = parallax
};

/*
 @brief رأس بسيط لتمرير مثلث/سطح شاشة كاملة.
 */
struct FullscreenVertex
{
    /*
     @brief الموضع الأفقي للرأس.
     */
    float x = 0.0f;
    /*
     @brief الموضع العمودي للرأس.
     */
    float y = 0.0f;
    /*
     @brief إحداثي UV الأفقي.
     */
    float u = 0.0f;
    /*
     @brief إحداثي UV العمودي.
     */
    float v = 0.0f;
};

/*
 @brief التجميع المركزي لتخطيطات ومخططات مؤثرات الجو.
 */
struct AtmospherePassLayouts
{
    /*
     @brief تخطيط descriptors الخاص بالشمس.
     */
    VkDescriptorSetLayout sunDescriptorSetLayout = VK_NULL_HANDLE;
    /*
     @brief تخطيط descriptors الخاص بأشعة الشمس.
     */
    VkDescriptorSetLayout godRayDescriptorSetLayout = VK_NULL_HANDLE;
    /*
     @brief Pipeline layout الخاص بالشمس.
     */
    VkPipelineLayout sunPipelineLayout = VK_NULL_HANDLE;
    /*
     @brief Pipeline layout الخاص بالأشعة.
     */
    VkPipelineLayout godRayPipelineLayout = VK_NULL_HANDLE;
};

class AtmosphereRenderer
{
  public:
#pragma region PublicInterface
    /*
     @brief ينشئ الكائن بحالة افتراضية فارغة.
     */
    AtmosphereRenderer() = default;

    /*
     @brief يحرر موارد الكائن عند انتهاء عمره.
     */
    ~AtmosphereRenderer() = default;

    AtmosphereRenderer(const AtmosphereRenderer&) = delete;
    AtmosphereRenderer& operator=(const AtmosphereRenderer&) = delete;

    /*
     @brief يهيئ مؤثرات الجو للامتداد الحالي للمشهد.
     @param device جهاز Vulkan المنطقي.
     @param sceneExtent أبعاد المشهد بالبكسل.
     */
    void init(VkDevice device, VkExtent2D sceneExtent);

    /*
     @brief ينظف الموارد المرتبطة بمؤثرات الجو.
     */
    void cleanup();

    /*
     @brief يحدث الامتداد عند تغيير الحجم.
     @param sceneExtent الأبعاد الجديدة للمشهد.
     */
    void onResize(VkExtent2D sceneExtent);

    /*
     @brief يكشف هل المؤثر مهيأ وجاهز للرسم.
     @return true إذا كانت التهيئة مكتملة.
     */
    [[nodiscard]] bool isInitialized() const;

    /*
     @brief يرجع التخطيطات ومخططات الأنابيب الجاهزة.
     @return مرجع ثابتة لتخطيطات ممرات الجو.
     */
    [[nodiscard]] const AtmospherePassLayouts& getLayouts() const;

    /*
     @brief يبني وصف blending إضافي لممرات الإضاءة.
     @return حالة الربط الإضافي المناسب للإضافة اللونية.
     */
    [[nodiscard]] static VkPipelineColorBlendAttachmentState makeAdditiveBlendAttachment();
#pragma endregion PublicInterface

  private:
#pragma region InternalHelpers
    /*
     @brief ينشئ تخطيطات descriptor sets.
     */
    void createDescriptorSetLayouts();

    /*
     @brief ينشئ pipeline layouts.
     */
    void createPipelineLayouts();

    /*
     @brief يدمر pipeline layouts الحالية.
     */
    void destroyPipelineLayouts();

    /*
     @brief يدمر descriptor set layouts الحالية.
     */
    void destroyDescriptorSetLayouts();
#pragma endregion InternalHelpers

#pragma region InternalState
    /*
     @brief جهاز Vulkan المستخدم.
     */
    VkDevice mDevice = VK_NULL_HANDLE;

    /*
     @brief أبعاد المشهد الحالية.
     */
    VkExtent2D mSceneExtent{};

    /*
     @brief مخازن التخطيطات المرتبطة بمؤثرات الجو.
     */
    AtmospherePassLayouts mLayouts{};

    /*
     @brief هل تمت التهيئة بنجاح؟
     */
    bool mInitialized = false;
#pragma endregion InternalState
};

} // namespace gfx
