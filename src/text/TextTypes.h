#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace textsys
{

/*
 * اتجاه النص المستخدم بين طبقات الـ BiDi والـ Layout.
 */
enum class TextDirection : std::uint8_t
{
    LeftToRight,
    RightToLeft,
    Neutral,
    Auto,
};

/*
 * محاذاة السطر على المحور الأفقي بعد اكتمال حساب العرض.
 * Start تعني أن originX هو بداية السطر.
 * End تعني أن originX هو نهاية السطر المناسبة لواجهات RTL.
 */
enum class TextAlignment : std::uint8_t
{
    Start,
    Center,
    End,
};

/*
 * تصنيف مبسط للسكريبت المستخدم في الـ shaping والـ layout.
 */
enum class Script : std::uint8_t
{
    Unknown,
    Arabic,
    Latin,
    Digit,
    Whitespace,
    Neutral,
};

/*
 * تصنيف محرف أولي أثناء تحليل النص الخام.
 */
enum class CharacterClass : std::uint8_t
{
    Arabic,
    Latin,
    Digit,
    Whitespace,
    Neutral,
};

/*
 * نوع اتصال الحرف العربي.
 */
enum class JoiningType : std::uint8_t
{
    NonJoining,
    RightJoining,
    DualJoining,
    Transparent,
};

/*
 * الشكل المختار للحرف بعد الـ shaping.
 */
enum class GlyphForm : std::uint8_t
{
    None,
    Isolated,
    Initial,
    Medial,
    Final,
    Ligature,
};

/*
 * النص بعد فك UTF-8 إلى Unicode codepoints.
 */
struct UnicodeText
{
    std::u32string codepoints;
};

/*
 * Run واحد قبل الـ shaping وبعد حسم الترتيب البصري بين الـ runs.
 * محتوى run يبقى بالترتيب المنطقي الداخلي، والـ layouter يقرر
 * إن كان يحتاج عكس ترتيب glyphs داخل run عند العرض.
 */
struct TextRun
{
    std::size_t logicalStart = 0;
    std::size_t logicalLength = 0;
    std::u32string text;
    Script script = Script::Unknown;
    TextDirection direction = TextDirection::Neutral;
    std::uint8_t embeddingLevel = 0;
};

/*
 * Glyph منطقي ناتج من الـ shaping.
 */
struct GlyphInfo
{
    char32_t sourceCodepoint = U'\0';
    char32_t glyphCodepoint = U'\0';
    Script script = Script::Unknown;
    TextDirection direction = TextDirection::Neutral;
    GlyphForm form = GlyphForm::None;
    std::size_t clusterStart = 0;
    std::size_t clusterLength = 1;
    bool visible = true;
};

/*
 * قياسات glyph كما يراها الـ layouter والـ renderer bridge.
 * لاحقاً يستطيع FreeTypeGlyphProvider ملء هذه البيانات من أطلس فعلي.
 */
struct GlyphMetrics
{
    float advanceX = 1.0f;
    float advanceY = 0.0f;
    float width = 1.0f;
    float height = 1.0f;
    float bearingX = 0.0f;
    float bearingY = 0.0f;
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
    bool hasRenderableQuad = true;
};

/*
 * Glyph بعد تثبيت موضعه داخل السطر.
 * penX / penY يمثلان موضع القلم baseline-origin قبل تطبيق bearing.
 */
struct PositionedGlyph
{
    GlyphInfo glyph;
    GlyphMetrics metrics;
    float penX = 0.0f;
    float penY = 0.0f;
    float advanceX = 0.0f;
    float advanceY = 0.0f;
};

/*
 * الناتج النهائي من مرحلة الـ layout لسطر واحد.
 */
struct TextLayout
{
    UnicodeText source;
    std::vector<TextRun> visualRuns;
    std::vector<PositionedGlyph> glyphs;
    float width = 0.0f;
    float height = 0.0f;
    float baseline = 0.0f;
};

/*
 * واجهة محلل الـ BiDi.
 * لاحقاً يمكن استبدالها بـ ICU من دون كسر بقية النظام.
 */
class IBidiResolver
{
  public:
    virtual ~IBidiResolver() = default;
    [[nodiscard]] virtual std::vector<TextRun> resolveVisualRuns(const UnicodeText& text) const = 0;
};

/*
 * واجهة الـ shaper.
 * لاحقاً يمكن استبدالها بـ HarfBuzzShaper.
 */
class ITextShaper
{
  public:
    virtual ~ITextShaper() = default;
    [[nodiscard]] virtual std::vector<GlyphInfo> shapeRun(const TextRun& run) const = 0;
};

/*
 * واجهة مزود قياسات glyphs.
 * لاحقاً يمكن استبدالها بـ FreeTypeGlyphProvider أو AtlasGlyphProvider.
 */
class IGlyphMetricsProvider
{
  public:
    virtual ~IGlyphMetricsProvider() = default;
    [[nodiscard]] virtual GlyphMetrics queryGlyphMetrics(const GlyphInfo& glyph) const = 0;
};

} // namespace textsys
