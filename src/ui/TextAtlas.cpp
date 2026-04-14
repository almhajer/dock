#include "TextAtlas.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string_view>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#endif

namespace ui::text
{
namespace
{

constexpr std::size_t kGlyphCount = static_cast<std::size_t>(GlyphId::Count);
constexpr int kAtlasPadding = 2;
constexpr int kGlyphPadding = 4;

enum class GlyphScript : unsigned char
{
    Latin,
    Arabic,
};

struct GlyphDefinition
{
    GlyphId id;
    std::string_view text;
    GlyphScript script;
    bool monospacedDigits = false;
    float fontSize = 34.0f;
};

struct RasterizedGlyph
{
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
};

struct CachedAtlas
{
    TextAtlas atlasData;
    std::array<GlyphMetrics, kGlyphCount> metrics = {};
};

constexpr std::array<GlyphDefinition, kGlyphCount> kGlyphDefinitions = {{
    {GlyphId::Digit0, "0", GlyphScript::Latin, true, 34.0f},
    {GlyphId::Digit1, "1", GlyphScript::Latin, true, 34.0f},
    {GlyphId::Digit2, "2", GlyphScript::Latin, true, 34.0f},
    {GlyphId::Digit3, "3", GlyphScript::Latin, true, 34.0f},
    {GlyphId::Digit4, "4", GlyphScript::Latin, true, 34.0f},
    {GlyphId::Digit5, "5", GlyphScript::Latin, true, 34.0f},
    {GlyphId::Digit6, "6", GlyphScript::Latin, true, 34.0f},
    {GlyphId::Digit7, "7", GlyphScript::Latin, true, 34.0f},
    {GlyphId::Digit8, "8", GlyphScript::Latin, true, 34.0f},
    {GlyphId::Digit9, "9", GlyphScript::Latin, true, 34.0f},
    {GlyphId::Slash, "/", GlyphScript::Latin, false, 34.0f},
    {GlyphId::Plus, "+", GlyphScript::Latin, false, 34.0f},
    {GlyphId::Minus, "-", GlyphScript::Latin, false, 34.0f},
    {GlyphId::Percent, "%", GlyphScript::Latin, false, 34.0f},
    {GlyphId::LatinA, "A", GlyphScript::Latin, false, 34.0f},
    {GlyphId::LatinD, "D", GlyphScript::Latin, false, 34.0f},
    {GlyphId::LatinE, "E", GlyphScript::Latin, false, 34.0f},
    {GlyphId::LatinN, "N", GlyphScript::Latin, false, 34.0f},
    {GlyphId::LatinR, "R", GlyphScript::Latin, false, 34.0f},
    {GlyphId::LatinT, "T", GlyphScript::Latin, false, 34.0f},
    {GlyphId::LatinV, "V", GlyphScript::Latin, false, 34.0f},
    {GlyphId::LatinX, "X", GlyphScript::Latin, false, 34.0f},
    {GlyphId::WordStage, "مرحلة", GlyphScript::Arabic, false, 38.0f},
    {GlyphId::WordHits, "إصابات", GlyphScript::Arabic, false, 38.0f},
    {GlyphId::WordShots, "طلقات", GlyphScript::Arabic, false, 38.0f},
    {GlyphId::WordAccuracy, "دقة", GlyphScript::Arabic, false, 38.0f},
    {GlyphId::WordPass, "نجاح", GlyphScript::Arabic, false, 38.0f},
    {GlyphId::WordFail, "فشل", GlyphScript::Arabic, false, 38.0f},
    {GlyphId::WordPoints, "نقاط", GlyphScript::Arabic, false, 38.0f},
    {GlyphId::WordPress, "اضغط", GlyphScript::Arabic, false, 38.0f},
    {GlyphId::WordVictory, "فوز", GlyphScript::Arabic, false, 40.0f},
    {GlyphId::WordContinue, "اضغط للمتابعة", GlyphScript::Arabic, false, 34.0f},
    {GlyphId::WordDucks, "بط", GlyphScript::Arabic, false, 36.0f},
    {GlyphId::WordBonus, "مكافأة", GlyphScript::Arabic, false, 36.0f},
    {GlyphId::WordChallenge, "تحدي", GlyphScript::Arabic, false, 36.0f},
    {GlyphId::WordBoss, "زعيم", GlyphScript::Arabic, false, 36.0f},
    {GlyphId::WordRare, "نادر", GlyphScript::Arabic, false, 36.0f},
}};

[[nodiscard]] constexpr std::size_t glyphIndex(GlyphId glyph)
{
    return static_cast<std::size_t>(glyph);
}

#ifdef __APPLE__
[[nodiscard]] CTFontRef createFontForGlyph(const GlyphDefinition& definition)
{
    if (definition.monospacedDigits)
    {
        return CTFontCreateWithName(CFSTR("Menlo-Bold"), definition.fontSize, nullptr);
    }

    const CFStringRef language = definition.script == GlyphScript::Arabic ? CFSTR("ar") : CFSTR("en");
    CTFontRef font = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, definition.fontSize, language);
    if (font != nullptr)
    {
        return font;
    }

    return CTFontCreateWithName(CFSTR("Helvetica"), definition.fontSize, nullptr);
}

[[nodiscard]] RasterizedGlyph rasterizeGlyph(const GlyphDefinition& definition)
{
    CTFontRef font = createFontForGlyph(definition);
    if (font == nullptr)
    {
        throw std::runtime_error("[TextAtlas] تعذر إنشاء الخط النصي");
    }

    CFStringRef text = CFStringCreateWithCString(kCFAllocatorDefault, definition.text.data(), kCFStringEncodingUTF8);
    if (text == nullptr)
    {
        CFRelease(font);
        throw std::runtime_error("[TextAtlas] تعذر تحويل النص إلى UTF-8");
    }

    CFMutableAttributedStringRef attributed = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);
    CFAttributedStringReplaceString(attributed, CFRangeMake(0, 0), text);

    const CFRange fullRange = CFRangeMake(0, CFStringGetLength(text));
    CFAttributedStringSetAttribute(attributed, fullRange, kCTFontAttributeName, font);
    CFAttributedStringSetAttribute(attributed, fullRange, kCTLanguageAttributeName,
                                   definition.script == GlyphScript::Arabic ? CFSTR("ar") : CFSTR("en"));

    const CTWritingDirection writingDirection =
        definition.script == GlyphScript::Arabic ? kCTWritingDirectionRightToLeft : kCTWritingDirectionLeftToRight;
    const CTParagraphStyleSetting paragraphSettings[] = {
        {kCTParagraphStyleSpecifierBaseWritingDirection, sizeof(writingDirection), &writingDirection},
    };
    CTParagraphStyleRef paragraphStyle = CTParagraphStyleCreate(paragraphSettings, 1);
    CFAttributedStringSetAttribute(attributed, fullRange, kCTParagraphStyleAttributeName, paragraphStyle);

    CTLineRef line = CTLineCreateWithAttributedString(attributed);

    /*
     * نعتمد على حدود الحبر الفعلية بدل القياسات الطباعية الخام،
     * لأن العربية قد تمتد يسار الأصل في سطر RTL فتظهر مقصوصة أو مزاحة.
     */
    const CGRect inkBounds = CTLineGetBoundsWithOptions(line, kCTLineBoundsUseGlyphPathBounds);
    const int bitmapWidth = std::max(1, static_cast<int>(std::ceil(inkBounds.size.width)) + kGlyphPadding * 2);
    const int bitmapHeight = std::max(1, static_cast<int>(std::ceil(inkBounds.size.height)) + kGlyphPadding * 2);

    std::vector<unsigned char> rawPixels(static_cast<std::size_t>(bitmapWidth * bitmapHeight * 4), 0);

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context =
        CGBitmapContextCreate(rawPixels.data(), bitmapWidth, bitmapHeight, 8, static_cast<std::size_t>(bitmapWidth) * 4,
                              colorSpace,
                              static_cast<CGBitmapInfo>(static_cast<uint32_t>(kCGImageAlphaPremultipliedLast) |
                                                         static_cast<uint32_t>(kCGBitmapByteOrder32Big)));
    if (context == nullptr)
    {
        CGColorSpaceRelease(colorSpace);
        CFRelease(line);
        CFRelease(paragraphStyle);
        CFRelease(attributed);
        CFRelease(text);
        CFRelease(font);
        throw std::runtime_error("[TextAtlas] تعذر إنشاء سياق الرسم النصي");
    }

    CGContextSetShouldAntialias(context, true);
    CGContextSetAllowsAntialiasing(context, true);
    CGContextSetTextMatrix(context, CGAffineTransformIdentity);
    CGContextSetRGBFillColor(context, 1.0, 1.0, 1.0, 1.0);
    const CGFloat drawX = static_cast<CGFloat>(kGlyphPadding) - inkBounds.origin.x;
    const CGFloat drawY = static_cast<CGFloat>(kGlyphPadding) - inkBounds.origin.y;
    CGContextSetTextPosition(context, drawX, drawY);
    CTLineDraw(line, context);
    CGContextFlush(context);

    RasterizedGlyph glyph;
    glyph.width = bitmapWidth;
    glyph.height = bitmapHeight;
    glyph.pixels.resize(rawPixels.size(), 0);

    const std::size_t rowBytes = static_cast<std::size_t>(bitmapWidth) * 4;
    for (int y = 0; y < bitmapHeight; ++y)
    {
        /*
         * بيانات CGBitmapContext تصل هنا بالفعل بترتيب الصفوف المناسب
         * لمسار textured quads الحالي، لذلك لا نقلب Y داخل الأطلس.
         */
        const unsigned char* srcRow = rawPixels.data() + static_cast<std::size_t>(y) * rowBytes;
        unsigned char* dstRow = glyph.pixels.data() + static_cast<std::size_t>(y) * rowBytes;

        for (int x = 0; x < bitmapWidth; ++x)
        {
            const std::size_t pixelOffset = static_cast<std::size_t>(x) * 4;

            /*
             * Quartz قد يكتب القنوات بترتيب داخلي مختلف عن المتوقع.
             * لذلك نحول النتيجة إلى mask صريح: RGB أبيض و alpha من أعلى تغطية.
             */
            const unsigned char coverage = std::max({srcRow[pixelOffset + 0], srcRow[pixelOffset + 1],
                                                     srcRow[pixelOffset + 2], srcRow[pixelOffset + 3]});
            dstRow[pixelOffset + 0] = 255;
            dstRow[pixelOffset + 1] = 255;
            dstRow[pixelOffset + 2] = 255;
            dstRow[pixelOffset + 3] = coverage;
        }
    }

    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);
    CFRelease(line);
    CFRelease(paragraphStyle);
    CFRelease(attributed);
    CFRelease(text);
    CFRelease(font);

    return glyph;
}
#else
[[nodiscard]] RasterizedGlyph rasterizeGlyph(const GlyphDefinition&)
{
    throw std::runtime_error("[TextAtlas] توليد النص الديناميكي مدعوم حالياً على macOS فقط");
}
#endif

[[nodiscard]] CachedAtlas buildAtlas()
{
    std::array<RasterizedGlyph, kGlyphCount> glyphBitmaps = {};

    int atlasWidth = 0;
    int atlasHeight = 0;
    for (const GlyphDefinition& definition : kGlyphDefinitions)
    {
        RasterizedGlyph glyph = rasterizeGlyph(definition);
        atlasWidth += glyph.width;
        atlasHeight = std::max(atlasHeight, glyph.height);
        glyphBitmaps[glyphIndex(definition.id)] = std::move(glyph);
    }

    atlasWidth += static_cast<int>(kGlyphDefinitions.size() - 1) * kAtlasPadding;

    CachedAtlas cached;
    cached.atlasData.width = atlasWidth;
    cached.atlasData.height = atlasHeight;
    cached.atlasData.pixels.resize(static_cast<std::size_t>(atlasWidth * atlasHeight * 4), 0);

    int penX = 0;
    for (const GlyphDefinition& definition : kGlyphDefinitions)
    {
        const RasterizedGlyph& glyph = glyphBitmaps[glyphIndex(definition.id)];
        const int offsetY = (atlasHeight - glyph.height) / 2;

        for (int y = 0; y < glyph.height; ++y)
        {
            const std::size_t srcOffset = static_cast<std::size_t>(y * glyph.width * 4);
            const std::size_t dstOffset = static_cast<std::size_t>(((offsetY + y) * atlasWidth + penX) * 4);
            std::memcpy(cached.atlasData.pixels.data() + dstOffset, glyph.pixels.data() + srcOffset,
                        static_cast<std::size_t>(glyph.width) * 4);
        }

        GlyphMetrics& metrics = cached.metrics[glyphIndex(definition.id)];
        metrics.u0 = static_cast<float>(penX) / static_cast<float>(atlasWidth);
        metrics.u1 = static_cast<float>(penX + glyph.width) / static_cast<float>(atlasWidth);
        metrics.v0 = static_cast<float>(offsetY) / static_cast<float>(atlasHeight);
        metrics.v1 = static_cast<float>(offsetY + glyph.height) / static_cast<float>(atlasHeight);
        metrics.pixelWidth = glyph.width;
        metrics.pixelHeight = glyph.height;

        penX += glyph.width + kAtlasPadding;
    }

    return cached;
}

[[nodiscard]] const CachedAtlas& cache()
{
    static const CachedAtlas kCached = buildAtlas();
    return kCached;
}

} // namespace

const TextAtlas& atlas()
{
    return cache().atlasData;
}

const GlyphMetrics& glyphMetrics(GlyphId glyph)
{
    return cache().metrics[glyphIndex(glyph)];
}

float glyphScreenWidth(GlyphId glyph, float glyphScreenHeight, float aspect)
{
    const GlyphMetrics& metrics = glyphMetrics(glyph);
    if (metrics.pixelWidth <= 0 || metrics.pixelHeight <= 0 || aspect <= 0.0f)
    {
        return 0.0f;
    }

    return glyphScreenHeight * (static_cast<float>(metrics.pixelWidth) / static_cast<float>(metrics.pixelHeight)) /
           aspect;
}

float measureGlyphs(const GlyphId* glyphs, int count, float glyphScreenHeight, float aspect, float gap)
{
    float width = 0.0f;
    for (int i = 0; i < count; ++i)
    {
        width += glyphScreenWidth(glyphs[i], glyphScreenHeight, aspect);
        if (i + 1 < count)
        {
            width += gap;
        }
    }
    return width;
}

void appendGlyphQuad(std::vector<gfx::TexturedQuad>& quads, GlyphId glyph, float& x, float y0, float glyphScreenHeight,
                     float aspect, float alpha, float gap)
{
    const GlyphMetrics& metrics = glyphMetrics(glyph);
    const float glyphWidth = glyphScreenWidth(glyph, glyphScreenHeight, aspect);

    gfx::TexturedQuad quad;
    quad.screen = gfx::makeScreenRect(x, x + glyphWidth, y0, y0 + glyphScreenHeight);
    quad.uv = {metrics.u0, metrics.u1, metrics.v0, metrics.v1};
    quad.alpha = alpha;
    quads.push_back(quad);

    x += glyphWidth + gap;
}

int writeNumberGlyphs(int value, GlyphId* out, int maxDigits)
{
    if (out == nullptr || maxDigits <= 0)
    {
        return 0;
    }

    if (value == 0)
    {
        out[0] = GlyphId::Digit0;
        return 1;
    }

    int digits[16];
    int count = 0;
    int temp = value;
    while (temp > 0 && count < maxDigits)
    {
        digits[count++] = temp % 10;
        temp /= 10;
    }

    for (int i = 0; i < count; ++i)
    {
        out[i] = static_cast<GlyphId>(glyphIndex(GlyphId::Digit0) + static_cast<std::size_t>(digits[count - 1 - i]));
    }

    return count;
}

} // namespace ui::text
