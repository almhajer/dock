/**
 * @file AsmaulHusnaOverlay.cpp
 * @brief تنفيذ تراكب أسماء الله الحسنى.
 * @details عرض أسماء الله الحسنى في اللعبة.
 */

#include "AsmaulHusnaOverlay.h"
#include "../rendering/SceneLayout.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#endif

namespace core
{
namespace
{

constexpr int kPadding = 4;
constexpr int kGridColumns = 10;

#pragma region JsonParser
std::string extractJsonString(const std::string& json, std::size_t& pos, const std::string& key)
{
    const std::string search = "\"" + key + "\"";
    const std::size_t keyPos = json.find(search, pos);
    if (keyPos == std::string::npos)
    {
        return {};
    }

    const std::size_t colonPos = json.find(':', keyPos + search.size());
    if (colonPos == std::string::npos)
    {
        return {};
    }

    const std::size_t quoteStart = json.find('"', colonPos + 1);
    if (quoteStart == std::string::npos)
    {
        return {};
    }

    const std::size_t quoteEnd = json.find('"', quoteStart + 1);
    if (quoteEnd == std::string::npos)
    {
        return {};
    }

    pos = quoteEnd + 1;
    return json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
}

std::vector<AsmaulHusnaOverlay::Entry> parseAsmaulHusnaJson(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "[AsmaulHusna] تعذر فتح الملف: " << filePath << std::endl;
        return {};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string json = buffer.str();

    std::vector<AsmaulHusnaOverlay::Entry> entries;
    std::set<std::string> seenMeaning;

    std::size_t pos = 0;
    while (true)
    {
        const std::size_t objStart = json.find('{', pos);
        if (objStart == std::string::npos)
        {
            break;
        }
        const std::size_t objEnd = json.find('}', objStart);
        if (objEnd == std::string::npos)
        {
            break;
        }

        std::size_t scanPos = objStart;
        const std::string meaningAr = extractJsonString(json, scanPos, "meaning_ar");
        scanPos = objStart;
        const std::string audioPath = extractJsonString(json, scanPos, "audio");

        if (!meaningAr.empty() && seenMeaning.find(meaningAr) == seenMeaning.end())
        {
            seenMeaning.insert(meaningAr);
            entries.push_back({meaningAr, audioPath});
        }

        pos = objEnd + 1;
    }

    return entries;
}
#pragma endregion JsonParser

#pragma region AudioResolver
/*
 * يبني قائمة مسارات الصوت الفعلية من ملفات mp3 الموجودة على القرص.
 * يربط كل ملف بالمدخل المناسب حسب الترتيب.
 */
std::vector<std::string> resolveAudioPaths(const std::string& audioDir)
{
    std::vector<std::string> files;
    namespace fs = std::filesystem;

    if (!fs::exists(audioDir))
    {
        std::cerr << "[AsmaulHusna] مجلد الصوت غير موجود: " << audioDir << std::endl;
        return {};
    }

    for (const auto& entry : fs::directory_iterator(audioDir))
    {
        if (entry.is_regular_file() && (entry.path().extension() == ".wav" || entry.path().extension() == ".mp3"))
        {
            files.push_back(entry.path().filename().string());
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}
#pragma endregion AudioResolver

#pragma region Rasterizer
#ifdef __APPLE__

struct RasterizedText
{
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
};

RasterizedText rasterizeArabicText(const std::string& text, float fontSize)
{
    CTFontRef font = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, fontSize, CFSTR("ar"));
    if (font == nullptr)
    {
        font = CTFontCreateWithName(CFSTR("Helvetica"), fontSize, nullptr);
    }

    CFStringRef cfText = CFStringCreateWithCString(kCFAllocatorDefault, text.c_str(), kCFStringEncodingUTF8);
    CFMutableAttributedStringRef attributed = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);
    CFAttributedStringReplaceString(attributed, CFRangeMake(0, 0), cfText);

    const CFRange fullRange = CFRangeMake(0, CFStringGetLength(cfText));
    CFAttributedStringSetAttribute(attributed, fullRange, kCTFontAttributeName, font);
    CFAttributedStringSetAttribute(attributed, fullRange, kCTLanguageAttributeName, CFSTR("ar"));

    const CTWritingDirection rtl = kCTWritingDirectionRightToLeft;
    const CTParagraphStyleSetting settings[] = {
        {kCTParagraphStyleSpecifierBaseWritingDirection, sizeof(rtl), &rtl},
    };
    CTParagraphStyleRef paraStyle = CTParagraphStyleCreate(settings, 1);
    CFAttributedStringSetAttribute(attributed, fullRange, kCTParagraphStyleAttributeName, paraStyle);

    CTLineRef line = CTLineCreateWithAttributedString(attributed);
    const CGRect inkBounds = CTLineGetBoundsWithOptions(line, kCTLineBoundsUseGlyphPathBounds);

    const int bmpW = std::max(1, static_cast<int>(std::ceil(inkBounds.size.width)) + kPadding * 2);
    const int bmpH = std::max(1, static_cast<int>(std::ceil(inkBounds.size.height)) + kPadding * 2);

    std::vector<unsigned char> rawPixels(static_cast<std::size_t>(bmpW * bmpH * 4), 0);

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    const CGBitmapInfo bitmapInfo = static_cast<CGBitmapInfo>(
        static_cast<uint32_t>(kCGImageAlphaPremultipliedLast) | static_cast<uint32_t>(kCGBitmapByteOrder32Big));
    CGContextRef ctx = CGBitmapContextCreate(rawPixels.data(), bmpW, bmpH, 8,
                                             static_cast<std::size_t>(bmpW) * 4, colorSpace, bitmapInfo);

    CGContextSetShouldAntialias(ctx, true);
    CGContextSetAllowsAntialiasing(ctx, true);
    CGContextSetTextMatrix(ctx, CGAffineTransformIdentity);
    CGContextSetRGBFillColor(ctx, 1.0, 1.0, 1.0, 1.0);
    CGContextSetTextPosition(ctx, static_cast<CGFloat>(kPadding) - inkBounds.origin.x,
                             static_cast<CGFloat>(kPadding) - inkBounds.origin.y);
    CTLineDraw(line, ctx);
    CGContextFlush(ctx);

    RasterizedText result;
    result.width = bmpW;
    result.height = bmpH;
    result.pixels.resize(rawPixels.size(), 0);

    const std::size_t rowBytes = static_cast<std::size_t>(bmpW) * 4;
    for (int y = 0; y < bmpH; ++y)
    {
        const unsigned char* src = rawPixels.data() + static_cast<std::size_t>(y) * rowBytes;
        unsigned char* dst = result.pixels.data() + static_cast<std::size_t>(y) * rowBytes;
        for (int x = 0; x < bmpW; ++x)
        {
            const std::size_t off = static_cast<std::size_t>(x) * 4;
            const unsigned char coverage = std::max({src[off + 0], src[off + 1], src[off + 2], src[off + 3]});
            dst[off + 0] = 255;
            dst[off + 1] = 255;
            dst[off + 2] = 255;
            dst[off + 3] = coverage;
        }
    }

    CGContextRelease(ctx);
    CGColorSpaceRelease(colorSpace);
    CFRelease(line);
    CFRelease(paraStyle);
    CFRelease(attributed);
    CFRelease(cfText);
    CFRelease(font);

    return result;
}

#endif
#pragma endregion Rasterizer

} // namespace

#pragma region Lifecycle
bool AsmaulHusnaOverlay::initialize(const std::string& assetsPath)
{
    const std::string jsonPath = assetsPath + "/data/asmaul_husna.json";
    mEntries = parseAsmaulHusnaJson(jsonPath);

    if (mEntries.empty())
    {
        std::cerr << "[AsmaulHusna] لم يتم العثور على بيانات" << std::endl;
        return false;
    }

    /*
     * حل مسارات الصوت من الملفات الفعلية على القرص بدلاً من JSON
     * لأن مسارات JSON لا تتطابق مع أسماء الملفات الموجودة.
     */
    const std::string audioDir = assetsPath + "/audio/asmaul_husna";
    const std::vector<std::string> audioFiles = resolveAudioPaths(audioDir);
    const int audioCount = static_cast<int>(audioFiles.size());
    const int entryCount = static_cast<int>(mEntries.size());
    for (int i = 0; i < entryCount && i < audioCount; ++i)
    {
        mEntries[static_cast<std::size_t>(i)].audioPath = audioDir + "/" + audioFiles[static_cast<std::size_t>(i)];
    }

    std::cout << "[AsmaulHusna] تم تحميل " << mEntries.size() << " اسم، " << audioCount << " ملف صوتي" << std::endl;

#ifdef __APPLE__
    const int count = static_cast<int>(mEntries.size());
    const int rows = (count + kGridColumns - 1) / kGridColumns;
    constexpr float kFontSize = 30.0f;
    constexpr int kCellPadding = 8;

    std::vector<RasterizedText> rasterized(static_cast<std::size_t>(count));
    int maxCellW = 0;
    int maxCellH = 0;

    for (int i = 0; i < count; ++i)
    {
        rasterized[static_cast<std::size_t>(i)] = rasterizeArabicText(mEntries[static_cast<std::size_t>(i)].meaningAr, kFontSize);
        maxCellW = std::max(maxCellW, rasterized[static_cast<std::size_t>(i)].width);
        maxCellH = std::max(maxCellH, rasterized[static_cast<std::size_t>(i)].height);
    }

    const int cellW = maxCellW + kCellPadding * 2;
    const int cellH = maxCellH + kCellPadding * 2;
    mTexW = cellW * kGridColumns;
    mTexH = cellH * rows;
    mPixels.resize(static_cast<std::size_t>(mTexW * mTexH * 4), 0);

    mGlyphs.resize(static_cast<std::size_t>(count));

    for (int i = 0; i < count; ++i)
    {
        const int col = i % kGridColumns;
        const int row = i / kGridColumns;
        const int cellX = col * cellW;
        const int cellY = row * cellH;

        const RasterizedText& rt = rasterized[static_cast<std::size_t>(i)];
        const int offX = cellX + (cellW - rt.width) / 2;
        const int offY = cellY + (cellH - rt.height) / 2;

        for (int y = 0; y < rt.height; ++y)
        {
            const std::size_t srcOff = static_cast<std::size_t>(y * rt.width * 4);
            const std::size_t dstOff = static_cast<std::size_t>(((offY + y) * mTexW + offX) * 4);
            if (offY + y >= 0 && offY + y < mTexH && offX + rt.width <= mTexW)
            {
                std::memcpy(mPixels.data() + dstOff, rt.pixels.data() + srcOff,
                            static_cast<std::size_t>(rt.width) * 4);
            }
        }

        GlyphUV& glyph = mGlyphs[static_cast<std::size_t>(i)];
        glyph.u0 = static_cast<float>(offX) / static_cast<float>(mTexW);
        glyph.u1 = static_cast<float>(offX + rt.width) / static_cast<float>(mTexW);
        glyph.v0 = static_cast<float>(offY) / static_cast<float>(mTexH);
        glyph.v1 = static_cast<float>(offY + rt.height) / static_cast<float>(mTexH);
        glyph.pixelWidth = rt.width;
        glyph.pixelHeight = rt.height;
    }

    std::cout << "[AsmaulHusna] تم بناء خامة " << mTexW << "x" << mTexH << std::endl;
    return true;
#else
    std::cerr << "[AsmaulHusna] دعم macOS فقط حالياً" << std::endl;
    return false;
#endif
}
#pragma endregion Lifecycle

#pragma region Gameplay
void AsmaulHusnaOverlay::onShot(audio::SoundEffectPlayer& primary, audio::SoundEffectPlayer& secondary,
                                 const std::string& /*assetsPath*/)
{
    if (mEntries.empty())
    {
        return;
    }

    const int entryIndex = mCurrentIndex;
    mCurrentIndex = (mCurrentIndex + 1) % static_cast<int>(mEntries.size());

    const Entry& entry = mEntries[static_cast<std::size_t>(entryIndex)];
    if (entry.audioPath.empty())
    {
        return;
    }

    auto& current = mPlayingSecondary ? secondary : primary;

    if (!current.isPlaying())
    {
        if (current.load(entry.audioPath))
        {
            current.play();
        }
    }
    else
    {
        mPendingAudio.push_back(entryIndex);
        preLoadNext(primary, secondary);
    }
}

void AsmaulHusnaOverlay::preLoadNext(audio::SoundEffectPlayer& primary, audio::SoundEffectPlayer& secondary)
{
    if (mPendingAudio.empty() || mNextPreLoaded >= 0)
    {
        return;
    }

    auto& other = mPlayingSecondary ? primary : secondary;
    if (other.isPlaying())
    {
        return;
    }

    const int idx = mPendingAudio.front();
    const Entry& e = mEntries[static_cast<std::size_t>(idx)];
    if (!e.audioPath.empty() && other.load(e.audioPath))
    {
        mNextPreLoaded = idx;
    }
}

void AsmaulHusnaOverlay::onHit(float screenX, float screenY)
{
    if (mEntries.empty())
    {
        return;
    }

    /*
     * نعرض معنى المدخل السابق (الذي شُغّل صوته مع الإطلاق)
     * لأن المؤشر تقدم في onShot، نرجع خطوة واحدة.
     */
    int textIndex = mCurrentIndex - 1;
    if (textIndex < 0)
    {
        textIndex = static_cast<int>(mEntries.size()) - 1;
    }

    mActiveEntry = textIndex;
    mTimer = kFadeDuration;
    mScreenX = screenX;
    mScreenY = screenY;
}

void AsmaulHusnaOverlay::update(float deltaTime, audio::SoundEffectPlayer& primary, audio::SoundEffectPlayer& secondary)
{
    /*
     * معالجة طابور الصوت بدون فراغات:
     * - المقطع التالي محمّل مسبقاً في المشغل الآخر
     * - عند انتهاء الحالي نبدأ الآخر فوراً ثم نحمّل الذي بعده
     */
    if (!mPendingAudio.empty())
    {
        auto& current = mPlayingSecondary ? secondary : primary;
        auto& other = mPlayingSecondary ? primary : secondary;

        if (!current.isPlaying())
        {
            if (mNextPreLoaded >= 0)
            {
                other.play();
                mNextPreLoaded = -1;
                mPendingAudio.erase(mPendingAudio.begin());
                mPlayingSecondary = !mPlayingSecondary;
                preLoadNext(primary, secondary);
            }
            else
            {
                const int idx = mPendingAudio.front();
                mPendingAudio.erase(mPendingAudio.begin());
                const Entry& e = mEntries[static_cast<std::size_t>(idx)];
                if (!e.audioPath.empty())
                {
                    if (current.load(e.audioPath))
                    {
                        current.play();
                    }
                }
                preLoadNext(primary, secondary);
            }
        }
    }

    if (mActiveEntry < 0)
    {
        return;
    }

    mTimer -= deltaTime;
    if (mTimer <= 0.0f)
    {
        mTimer = 0.0f;
        mActiveEntry = -1;
    }
}

bool AsmaulHusnaOverlay::isActive() const
{
    return mActiveEntry >= 0;
}

int AsmaulHusnaOverlay::currentIndex() const
{
    return mCurrentIndex;
}

void AsmaulHusnaOverlay::setCurrentIndex(int index)
{
    if (mEntries.empty())
    {
        return;
    }
    mCurrentIndex = index % static_cast<int>(mEntries.size());
    if (mCurrentIndex < 0)
    {
        mCurrentIndex += static_cast<int>(mEntries.size());
    }
}
#pragma endregion Gameplay

#pragma region RenderData
const std::vector<unsigned char>& AsmaulHusnaOverlay::texturePixels() const
{
    return mPixels;
}

int AsmaulHusnaOverlay::textureWidth() const
{
    return mTexW;
}

int AsmaulHusnaOverlay::textureHeight() const
{
    return mTexH;
}

void AsmaulHusnaOverlay::buildTextQuads(std::vector<gfx::TexturedQuad>& quads, float aspect) const
{
    if (mActiveEntry < 0 || mActiveEntry >= static_cast<int>(mGlyphs.size()))
    {
        return;
    }

    const float alpha = std::clamp(mTimer / kFadeDuration, 0.0f, 1.0f);
    const GlyphUV& glyph = mGlyphs[static_cast<std::size_t>(mActiveEntry)];

    const float screenH = kScreenH;
    const float screenW = screenH * (static_cast<float>(glyph.pixelWidth) / static_cast<float>(glyph.pixelHeight)) / aspect;

    /*
     * محاذاة الموضع كي لا يخرج النص خارج حدود النافذة.
     * النص متمركز حول مكان الإصابة، لكن نزاحة إن لزم الأمر.
     */
    float cx = mScreenX;
    if (cx - screenW * 0.5f < -1.0f + kMargin)
    {
        cx = -1.0f + kMargin + screenW * 0.5f;
    }
    else if (cx + screenW * 0.5f > 1.0f - kMargin)
    {
        cx = 1.0f - kMargin - screenW * 0.5f;
    }

    float cy = mScreenY;
    if (cy - screenH * 0.5f < -1.0f + kMargin)
    {
        cy = -1.0f + kMargin + screenH * 0.5f;
    }
    else if (cy + screenH * 0.5f > 1.0f - kMargin)
    {
        cy = 1.0f - kMargin - screenH * 0.5f;
    }

    const float x0 = cx - screenW * 0.5f;
    const float x1 = cx + screenW * 0.5f;
    const float y0 = cy - screenH * 0.5f;
    const float y1 = cy + screenH * 0.5f;

    gfx::TexturedQuad quad;
    quad.screen = gfx::makeScreenRect(x0, x1, y0, y1);
    quad.uv = {glyph.u0, glyph.u1, glyph.v0, glyph.v1};
    quad.alpha = alpha;
    quads.push_back(quad);
}

void AsmaulHusnaOverlay::buildBgQuad(std::vector<gfx::TexturedQuad>& quads, float aspect) const
{
    if (mActiveEntry < 0 || mActiveEntry >= static_cast<int>(mGlyphs.size()))
    {
        return;
    }

    const float alpha = std::clamp(mTimer / kFadeDuration, 0.0f, 1.0f) * 0.55f;
    const GlyphUV& glyph = mGlyphs[static_cast<std::size_t>(mActiveEntry)];

    const float screenH = kScreenH;
    const float screenW = screenH * (static_cast<float>(glyph.pixelWidth) / static_cast<float>(glyph.pixelHeight)) / aspect;

    float cx = mScreenX;
    if (cx - screenW * 0.5f < -1.0f + kMargin)
    {
        cx = -1.0f + kMargin + screenW * 0.5f;
    }
    else if (cx + screenW * 0.5f > 1.0f - kMargin)
    {
        cx = 1.0f - kMargin - screenW * 0.5f;
    }

    float cy = mScreenY;
    if (cy - screenH * 0.5f < -1.0f + kMargin)
    {
        cy = -1.0f + kMargin + screenH * 0.5f;
    }
    else if (cy + screenH * 0.5f > 1.0f - kMargin)
    {
        cy = 1.0f - kMargin - screenH * 0.5f;
    }

    const float x0 = cx - screenW * 0.5f - kBgPadX;
    const float x1 = cx + screenW * 0.5f + kBgPadX;
    const float y0 = cy - screenH * 0.5f - kBgPadY;
    const float y1 = cy + screenH * 0.5f + kBgPadY;

    gfx::TexturedQuad quad;
    quad.screen = gfx::makeScreenRect(x0, x1, y0, y1);
    quad.uv = gfx::fullUvRect();
    quad.alpha = alpha;
    quads.push_back(quad);
}
#pragma endregion RenderData

} // namespace core
