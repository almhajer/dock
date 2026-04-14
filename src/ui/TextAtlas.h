#pragma once

#include "../gfx/RenderTypes.h"

#include <cstdint>
#include <vector>

namespace ui::text
{

/*
 * معرفات الرموز الموجودة داخل أطلس النص.
 * جمعنا الكلمات العربية كاملة كوحدات مستقلة لأن نظام الرسم الحالي
 * يعتمد على quads وخامة واحدة، ولا يملك محرك تشكيل عربي داخلي.
 */
enum class GlyphId : std::uint16_t
{
    Digit0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,
    Slash,
    Plus,
    Minus,
    Percent,
    LatinA,
    LatinD,
    LatinE,
    LatinN,
    LatinR,
    LatinT,
    LatinV,
    LatinX,
    WordStage,
    WordHits,
    WordShots,
    WordAccuracy,
    WordPass,
    WordFail,
    WordPoints,
    WordPress,
    WordVictory,
    WordContinue,
    WordDucks,
    WordBonus,
    WordChallenge,
    WordBoss,
    WordRare,
    WordPaused,
    Count,
};

/*
 * معلومات موضع الرمز داخل الأطلس مع أبعاده الفعلية.
 */
struct GlyphMetrics
{
    float u0 = 0.0f;
    float u1 = 0.0f;
    float v0 = 0.0f;
    float v1 = 0.0f;
    int pixelWidth = 0;
    int pixelHeight = 0;
};

/*
 * بيانات خامة النص الجاهزة للرفع إلى Vulkan.
 */
struct TextAtlas
{
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
};

/*
 * يرجع الأطلس النصي المشترك لكل واجهات اللعبة.
 */
[[nodiscard]] const TextAtlas& atlas();

/*
 * يرجع معلومات UV والحجم الخاصة برمز واحد.
 */
[[nodiscard]] const GlyphMetrics& glyphMetrics(GlyphId glyph);

/*
 * يحسب العرض المنطقي على الشاشة لرمز واحد بحسب ارتفاعه المطلوب.
 */
[[nodiscard]] float glyphScreenWidth(GlyphId glyph, float glyphScreenHeight, float aspect);

/*
 * يحسب العرض الكلي لتسلسل رموز مع فراغ ثابت بينها.
 */
[[nodiscard]] float measureGlyphs(const GlyphId* glyphs, int count, float glyphScreenHeight, float aspect, float gap);

/*
 * يضيف quad واحد إلى قائمة الرسم ويحرك المؤشر الأفقي إلى الموضع التالي.
 */
void appendGlyphQuad(std::vector<gfx::TexturedQuad>& quads, GlyphId glyph, float& x, float y0, float glyphScreenHeight,
                     float aspect, float alpha, float gap);

/*
 * يحول عدداً صحيحاً إلى رموز أرقام داخل المصفوفة المعطاة.
 * يرجع عدد الرموز المكتوبة فعلياً.
 */
[[nodiscard]] int writeNumberGlyphs(int value, GlyphId* out, int maxDigits);

} // namespace ui::text
