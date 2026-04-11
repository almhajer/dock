#pragma once

#include "../game/SpriteAtlas.h"
#include "../gfx/RenderTypes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace core::scene {

#pragma region LayoutConstants
/*
 @brief الحد الأعلى لعدد quads العشب التي تبنى لكل إطار عرض.
 */
inline constexpr std::size_t kMaxGrassQuads = 240;
#pragma endregion LayoutConstants

#pragma region LayoutTypes
/*
 @brief يحدد طبقة العمق المراد بناء العشب لها.
 */
enum class GrassDepthLayer {
    Background,
    Midground,
    Foreground,
};

/*
 @brief القياسات المشتقة من حجم النافذة لاستعمالها في الحسابات المنطقية.
 */
struct WindowMetrics {
    /*
     @brief عرض النافذة بالبكسل.
     */
    uint32_t width = 0;

    /*
     @brief ارتفاع النافذة بالبكسل.
     */
    uint32_t height = 0;

    /*
     @brief عرض النافذة كقيمة عشرية.
     */
    float widthF = 0.0f;

    /*
     @brief ارتفاع النافذة كقيمة عشرية.
     */
    float heightF = 0.0f;

    /*
     @brief نسبة العرض إلى الارتفاع.
     */
    float aspect = 1.0f;

    /*
     @brief يتحقق أن المقاسات صالحة لبناء التخطيط.
     */
    [[nodiscard]] bool valid() const {
        return width > 0 && height > 0 && widthF >= 1.0f && heightF >= 1.0f;
    }
};

/*
 @brief بيانات توزيع بلاطات العشب قبل تحويلها إلى quads فعلية.
 */
struct GrassLayout {
    /*
     @brief عرض البلاطة المنطقية الواحدة.
     */
    float tileWidth = 0.0f;

    /*
     @brief ارتفاع البلاطة المنطقية الواحدة.
     */
    float tileHeight = 0.0f;

    /*
     @brief خطوة الانتقال بين البلاطات المتجاورة.
     */
    float tileStep = 0.0f;

    /*
     @brief نقطة البداية الأفقية لأول بلاطة.
     */
    float startX = 0.0f;

    /*
     @brief عدد البلاطات المطلوبة لتغطية العرض بالكامل.
     */
    int tileCount = 0;
};
#pragma endregion LayoutTypes

#pragma region MetricHelpers
/*
 @brief يبني قياسات النافذة المنطقية من العرض والارتفاع الحاليين.
 @param width عرض النافذة بالبكسل.
 @param height ارتفاع النافذة بالبكسل.
 @return قياسات النافذة المنطقية.
 */
WindowMetrics makeWindowMetrics(uint32_t width, uint32_t height);

/*
 @brief يقارن بين التخطيط الحالي والمحفوظ لتفادي إعادة البناء دون داع.
 @param metrics قياسات النافذة الحالية.
 @param cachedWidth العرض المحفوظ سابقاً.
 @param cachedHeight الارتفاع المحفوظ سابقاً.
 @return true إذا كانت الأبعاد متطابقة.
 */
bool sameWindowLayout(const WindowMetrics& metrics, uint32_t cachedWidth, uint32_t cachedHeight);

/*
 @brief يحسب نصف عرض السبرايت المنطقي انطلاقاً من أبعاد الفريم الأصلية.
 @param frame فريم الأطلس المرجعي.
 @param metrics قياسات النافذة المنطقية.
 @param logicalHalfHeight نصف الارتفاع المنطقي.
 @return نصف العرض المنطقي.
 */
float spriteLogicalHalfWidth(const game::AtlasFrame& frame,
                             const WindowMetrics& metrics,
                             float logicalHalfHeight);

/*
 @brief يحسب نصف عرض الصياد المنطقي بالاعتماد على نفس قواعد السبرايت العامة.
 @param frame فريم الأطلس المرجعي.
 @param metrics قياسات النافذة المنطقية.
 @return نصف العرض المنطقي للصياد.
 */
float hunterLogicalHalfWidth(const game::AtlasFrame& frame, const WindowMetrics& metrics);

/*
 @brief يرجع موضع سطح الأرض المرئي في الإحداثيات المنطقية.
 @return الإحداثي العمودي لسطح الأرض.
 */
float groundSurfaceY();

/*
 @brief يرجع الحد العلوي لشريط العشب.
 @return الإحداثي العمودي لحد العشب.
 */
float grassTopY();
#pragma endregion MetricHelpers

#pragma region QuadBuilders
/*
 @brief يبني quad التربة الإجرائي الذي يغطي أسفل الشاشة.
 @return quad التربة الجاهز للرسم.
 */
gfx::TexturedQuad buildSoilQuad();

/*
 @brief يبني quad لأي سبرايت اعتماداً على pivot الفريم وحجمه المنطقي.
 @param frame فريم الأطلس المرجعي.
 @param pivotScreenX الإحداثي الأفقي لنقطة الارتكاز على الشاشة.
 @param pivotScreenY الإحداثي العمودي لنقطة الارتكاز على الشاشة.
 @param logicalHalfHeight نصف الارتفاع المنطقي.
 @param metrics قياسات النافذة المنطقية.
 @return quad السبرايت الجاهز للرسم.
 */
gfx::TexturedQuad buildSpriteQuad(const game::AtlasFrame& frame,
                                  float pivotScreenX,
                                  float pivotScreenY,
                                  float logicalHalfHeight,
                                  const WindowMetrics& metrics);

/*
 @brief يبني quad الصياد بمحاذاة الأرض الحالية.
 @param frame فريم الأطلس المرجعي.
 @param hunterScreenX الإحداثي الأفقي للصياد على الشاشة.
 @param metrics قياسات النافذة المنطقية.
 @return quad الصياد الجاهز للرسم.
 */
gfx::TexturedQuad buildHunterQuad(const game::AtlasFrame& frame,
                                  float hunterScreenX,
                                  const WindowMetrics& metrics);
#pragma endregion QuadBuilders

#pragma region GrassBuilders
/*
 @brief يبني تخطيط توزيع العشب حسب أبعاد النافذة الحالية.
 @param metrics قياسات النافذة المنطقية.
 @return تخطيط العشب.
 */
GrassLayout buildGrassLayout(const WindowMetrics& metrics);

/*
 @brief يضيف دفعة quads خاصة بطبقة عشب محددة إلى الحاوية الهدف.
 @param quads الحاوية التي تُضاف إليها quads العشب.
 @param layout تخطيط العشب.
 @param tileIndex فهرس البلاطة.
 @param layer طبقة العمق.
 @param maxQuads الحد الأقصى لعدد quads.
 */
void appendGrassQuads(std::vector<gfx::TexturedQuad>& quads,
                      const GrassLayout& layout,
                      int tileIndex,
                      GrassDepthLayer layer,
                      std::size_t maxQuads = kMaxGrassQuads);
#pragma endregion GrassBuilders

#pragma region FootPlacement
/*
 @brief يحسب طور المشية المعتمد في تأثيرات القدمين.
 @param animationTime زمن الرسم المتحرك.
 @return طور المشية بين 0 و 1.
 */
float resolveGait(float animationTime);

/*
 @brief يحسب ضغط القدم الحالية على الأرض.
 @param gait طور المشية.
 @param leftFoot هل القدم اليسرى هي النشطة؟
 @return قيمة الضغط بين 0 و 1.
 */
float resolvePressure(float gait, bool leftFoot);

/*
 @brief يحسب الموقع الأفقي للقدم النشطة حول مركز الصياد.
 @param hunterScreenX الإحداثي الأفقي للصياد على الشاشة.
 @param hunterLogicalWidth العرض المنطقي الكلي للصياد.
 @param gait طور المشية.
 @param leftFoot هل القدم اليسرى هي النشطة؟
 @return الموقع الأفقي للقدم.
 */
float resolveFootTargetX(float hunterScreenX, float hunterLogicalWidth, float gait, bool leftFoot);
#pragma endregion FootPlacement

} // namespace core::scene
