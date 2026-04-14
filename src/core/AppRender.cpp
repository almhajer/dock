#include "App.h"
#include "DuckGameplay.h"
#include "HunterGameplay.h"
#include "SceneLayout.h"
#include "../ui/TextAtlas.h"

#include <algorithm>
#include <array>
#include <vector>

namespace core
{
namespace
{

/*
 * ثوابت آثار القدمين معزولة هنا لأنها تخص العرض الأرضي فقط
 * ولا نحتاجها في ملفات اللعب الأخرى.
 */
constexpr float kFootprintDecayPerSecond = 1.85f;
constexpr float kFootprintFollowPerSecond = 18.0f;
constexpr float kFootContactThreshold = 0.08f;

using UiGlyph = ui::text::GlyphId;

struct HudLayout
{
    float panelX0 = -0.27f;
    float panelX1 = 0.27f;
    float panelY0 = -0.972f;
    float panelY1 = -0.888f;
    float stageRightX = -0.33f;
    float ducksRightX = 0.56f;
    float statsRightX = 0.93f;
    float primaryRowY0 = -0.955f;
    float secondaryRowY0 = -0.860f;
    float tertiaryRowY0 = -0.800f;
};

struct ResultsLayout
{
    float panelX0 = -0.34f;
    float panelX1 = 0.34f;
    float panelY0 = -0.38f;
    float panelY1 = 0.38f;
    float titleY0 = -0.300f;
    float subtitleY0 = -0.180f;
    float metricsStartY = -0.155f;
    float metricGap = 0.084f;
    float labelRightX = 0.24f;
    float valueRightX = -0.05f;
    float footerY0 = 0.275f;
};

[[nodiscard]] HudLayout makeHudLayout()
{
    HudLayout layout;
    constexpr float kPrimaryRowHeight = 0.050f;
    layout.primaryRowY0 = ((layout.panelY0 + layout.panelY1) * 0.5f) - kPrimaryRowHeight * 0.5f;
    layout.secondaryRowY0 = layout.panelY1 + 0.008f;
    layout.tertiaryRowY0 = layout.secondaryRowY0 + 0.055f;
    layout.stageRightX = layout.panelX0 - 0.055f;
    return layout;
}

[[nodiscard]] ResultsLayout makeResultsLayout(bool hasBonusLine)
{
    ResultsLayout layout;
    layout.metricGap = hasBonusLine ? 0.078f : 0.090f;
    return layout;
}

/*
 * هذه المساعدات توحد قياس النص ورسمه فوق الأطلس الجديد،
 * حتى يبقى AppRender مركزاً على التموضع لا على تفاصيل الـ UV.
 */
[[nodiscard]] float measureGlyphRun(const UiGlyph* glyphs, int count, float height, float aspect, float gap)
{
    return ui::text::measureGlyphs(glyphs, count, height, aspect, gap);
}

void appendGlyphRun(std::vector<gfx::TexturedQuad>& quads, const UiGlyph* glyphs, int count, float& x, float y0,
                    float height, float aspect, float alpha, float gap)
{
    for (int i = 0; i < count; ++i)
    {
        ui::text::appendGlyphQuad(quads, glyphs[i], x, y0, height, aspect, alpha, gap);
    }
}

template <std::size_t N>
[[nodiscard]] int writeNumberRun(int value, std::array<UiGlyph, N>& out)
{
    return ui::text::writeNumberGlyphs(value, out.data(), static_cast<int>(out.size()));
}

void appendRightAnchoredArabicWithNumbers(std::vector<gfx::TexturedQuad>& quads, UiGlyph wordGlyph,
                                          const UiGlyph* numberGlyphs, int numberCount, float rightX, float rowTopY,
                                          float wordHeight, float numberHeight, float aspect, float wordAlpha,
                                          float numberAlpha, float gap)
{
    const UiGlyph wordRun[] = {wordGlyph};
    const float rowHeight = std::max(wordHeight, numberHeight);
    const float wordWidth = measureGlyphRun(wordRun, 1, wordHeight, aspect, gap);
    const float numberWidth = measureGlyphRun(numberGlyphs, numberCount, numberHeight, aspect, gap);
    const float spaceWidth = ui::text::glyphScreenWidth(UiGlyph::Digit0, rowHeight, aspect) * 0.42f;

    float x = rightX - (numberWidth + spaceWidth + wordWidth);
    float numberX = x;
    appendGlyphRun(quads, numberGlyphs, numberCount, numberX, rowTopY + (rowHeight - numberHeight) * 0.5f,
                   numberHeight, aspect, numberAlpha, gap);

    float wordX = x + numberWidth + spaceWidth;
    appendGlyphRun(quads, wordRun, 1, wordX, rowTopY + (rowHeight - wordHeight) * 0.5f, wordHeight, aspect,
                   wordAlpha, gap);
}

void appendMetricRow(std::vector<gfx::TexturedQuad>& quads, UiGlyph labelGlyph, const UiGlyph* valueGlyphs,
                     int valueCount, float labelRightX, float valueRightX, float rowTopY, float labelHeight,
                     float valueHeight, float aspect, float labelAlpha, float valueAlpha, float gap)
{
    const UiGlyph labelRun[] = {labelGlyph};
    const float rowHeight = std::max(labelHeight, valueHeight);
    float valueX = valueRightX - measureGlyphRun(valueGlyphs, valueCount, valueHeight, aspect, gap);
    float labelX = labelRightX - measureGlyphRun(labelRun, 1, labelHeight, aspect, gap);

    appendGlyphRun(quads, valueGlyphs, valueCount, valueX, rowTopY + (rowHeight - valueHeight) * 0.5f, valueHeight,
                   aspect, valueAlpha, gap);
    appendGlyphRun(quads, labelRun, 1, labelX, rowTopY + (rowHeight - labelHeight) * 0.5f, labelHeight, aspect,
                   labelAlpha, gap);
}

template <std::size_t N>
[[nodiscard]] int writeFractionRun(int leftValue, int rightValue, std::array<UiGlyph, N>& out)
{
    std::array<UiGlyph, N> leftDigits = {};
    std::array<UiGlyph, N> rightDigits = {};
    const int leftCount = writeNumberRun(leftValue, leftDigits);
    const int rightCount = writeNumberRun(rightValue, rightDigits);

    int count = 0;
    for (int i = 0; i < leftCount && count < static_cast<int>(out.size()); ++i)
    {
        out[static_cast<std::size_t>(count++)] = leftDigits[static_cast<std::size_t>(i)];
    }
    if (count < static_cast<int>(out.size()))
    {
        out[static_cast<std::size_t>(count++)] = UiGlyph::Slash;
    }
    for (int i = 0; i < rightCount && count < static_cast<int>(out.size()); ++i)
    {
        out[static_cast<std::size_t>(count++)] = rightDigits[static_cast<std::size_t>(i)];
    }
    return count;
}

} // namespace

/*
 * هذا الملف يجمع كل ما يخص تجهيز طبقات العرض قبل الرسم النهائي:
 * - الأرض والعشب
 * - الصياد والبطة
 * - تفاعل القدمين مع الأرض
 */

void App::updateSoilRenderData()
{
    if (mSoilLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
    {
        return;
    }

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        mSoilLayoutWidth = 0;
        mSoilLayoutHeight = 0;
        mVulkan.clearTexturedLayer(mSoilLayerId);
        return;
    }

    if (scene::sameWindowLayout(metrics, mSoilLayoutWidth, mSoilLayoutHeight))
    {
        return;
    }

    mSoilLayoutWidth = metrics.width;
    mSoilLayoutHeight = metrics.height;
    mVulkan.updateTexturedLayer(mSoilLayerId, scene::buildSoilQuad());
}

void App::updateGrassRenderData()
{
    if (mGrassLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
    {
        return;
    }

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        mGrassLayoutWidth = 0;
        mGrassLayoutHeight = 0;
        mVulkan.clearTexturedLayer(mGrassLayerId);
        return;
    }
    if (scene::sameWindowLayout(metrics, mGrassLayoutWidth, mGrassLayoutHeight))
    {
        return;
    }

    mGrassLayoutWidth = metrics.width;
    mGrassLayoutHeight = metrics.height;
    const scene::GrassLayout layout = scene::buildGrassLayout(metrics);

    std::vector<gfx::TexturedQuad> quads;
    quads.reserve(scene::kMaxGrassQuads);

    constexpr std::array<scene::GrassDepthLayer, 3> GRASS_LAYERS = {
        scene::GrassDepthLayer::Background,
        scene::GrassDepthLayer::Midground,
        scene::GrassDepthLayer::Foreground,
    };

    for (scene::GrassDepthLayer layer : GRASS_LAYERS)
    {
        for (int i = 0; i < layout.tileCount && quads.size() < scene::kMaxGrassQuads; ++i)
        {
            scene::appendGrassQuads(quads, layout, i, layer);
        }
    }

    mVulkan.updateTexturedLayer(mGrassLayerId, quads);
}

void App::updateNatureRenderData()
{
    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        mEnvironmentRenderData.cloudInstances.clear();
        mEnvironmentRenderData.backgroundTreeInstances.clear();
        mEnvironmentRenderData.foregroundTreeInstances.clear();
        mVulkan.updateEnvironmentBatches(mEnvironmentRenderData.cloudInstances,
                                         mEnvironmentRenderData.backgroundTreeInstances,
                                         mEnvironmentRenderData.foregroundTreeInstances);
        return;
    }

    mNatureSystem.buildRenderData(metrics, mEnvironmentRenderData);
    mVulkan.updateEnvironmentBatches(mEnvironmentRenderData.cloudInstances,
                                     mEnvironmentRenderData.backgroundTreeInstances,
                                     mEnvironmentRenderData.foregroundTreeInstances);
}

void App::updateHunterRenderData()
{
    if (mHunterLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
    {
        return;
    }

    if (mHunterState.currentClip.empty())
    {
        return;
    }

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        return;
    }

    const game::AtlasFrame* frame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
    if (frame == nullptr || frame->sourceW <= 0 || frame->sourceH <= 0)
    {
        mVulkan.clearTexturedLayer(mHunterLayerId);
        return;
    }

    gfx::UvRect uv;
    mSpriteAnim.getFrameUV(mHunterState.currentFrameIndex, uv.u0, uv.u1, uv.v0, uv.v1);
    if (mHunterState.flipX)
    {
        std::swap(uv.u0, uv.u1);
    }

    const float logicalHalfWidth = scene::hunterLogicalHalfWidth(*frame, metrics);
    const float clampMin = -1.0f + logicalHalfWidth;
    const float clampMax = 1.0f - logicalHalfWidth;
    if (clampMin < clampMax)
    {
        mHunter.x = std::clamp(mHunter.x, clampMin, clampMax);
    }

    gfx::TexturedQuad quad = scene::buildHunterQuad(*frame, mHunter.x, metrics);
    quad.uv = uv;
    mVulkan.updateTexturedLayer(mHunterLayerId, quad);
}

void App::updateDuckRenderData()
{
    if (mDuckLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
    {
        return;
    }

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid() || !mDuckPool.hasAnyActive())
    {
        mVulkan.clearTexturedLayer(mDuckLayerId);
        return;
    }

    std::vector<gfx::TexturedQuad> quads;
    mDuckPool.buildRenderQuads(quads, metrics);
    if (quads.empty())
    {
        mVulkan.clearTexturedLayer(mDuckLayerId);
    }
    else
    {
        mVulkan.updateTexturedLayer(mDuckLayerId, quads);
    }
}

void App::updateScoreRenderData()
{
    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        if (mScoreBgLayerId != gfx::VulkanContext::INVALID_LAYER_ID)
        {
            mVulkan.clearTexturedLayer(mScoreBgLayerId);
        }
        if (mScoreFillLayerId != gfx::VulkanContext::INVALID_LAYER_ID)
        {
            mVulkan.clearTexturedLayer(mScoreFillLayerId);
        }
        if (mScoreLayerId != gfx::VulkanContext::INVALID_LAYER_ID)
        {
            mVulkan.clearTexturedLayer(mScoreLayerId);
        }
        return;
    }

    const HudLayout hudLayout = makeHudLayout();
    constexpr float kDigitScreenH = 0.058f;
    constexpr float kGap = 0.006f;
    constexpr float kBarPadX = 0.008f;
    constexpr float kBarPadY = 0.010f;

    std::array<UiGlyph, 12> digits = {};
    const int digitCount = writeFractionRun(mStageState.ducksHitThisStage,
                                            stage::kStageTable[mStageState.currentStageIndex].totalDucks, digits);
    const float numberWidth = measureGlyphRun(digits.data(), digitCount, kDigitScreenH, metrics.aspect, kGap);

    // اللوحة الخارجية (ثابتة الحجم)
    const float panelX0 = hudLayout.panelX0;
    const float panelX1 = hudLayout.panelX1;
    const float panelY0 = hudLayout.panelY0;
    const float panelY1 = hudLayout.panelY1;

    if (mScoreBgLayerId != gfx::VulkanContext::INVALID_LAYER_ID)
    {
        const gfx::TexturedQuad bgQuad = gfx::makeTexturedQuad(
            gfx::makeScreenRect(panelX0, panelX1, panelY0, panelY1), gfx::fullUvRect(), 1.0f);
        mVulkan.updateTexturedLayer(mScoreBgLayerId, bgQuad);
    }

    // الشريط الداخلي (يكبر مع الإصابات)
    const auto &stageConfig = stage::kStageTable[mStageState.currentStageIndex];
    const float fillMax = static_cast<float>(std::max(stageConfig.totalDucks, 1));
    const float fillFraction = std::min(static_cast<float>(mStageState.ducksHitThisStage), fillMax) / fillMax;
    const float barMaxWidth = (panelX1 - panelX0) - 2.0f * kBarPadX;
    const float barWidth = std::max(numberWidth + 0.012f, fillFraction * barMaxWidth);
    const float barX0 = panelX0 + kBarPadX;
    const float barX1 = std::min(barX0 + barWidth, panelX1 - kBarPadX);
    const float barY0 = panelY0 + kBarPadY;
    const float barY1 = panelY1 - kBarPadY;

    if (mScoreFillLayerId != gfx::VulkanContext::INVALID_LAYER_ID)
    {
        const gfx::TexturedQuad fillQuad = gfx::makeTexturedQuad(
            gfx::makeScreenRect(barX0, barX1, barY0, barY1), gfx::fullUvRect(), 1.0f);
        mVulkan.updateTexturedLayer(mScoreFillLayerId, fillQuad);
    }

    // الأرقام (تتمركز على الشريط الداخلي وتتحرك مع نموه)
    if (mScoreLayerId != gfx::VulkanContext::INVALID_LAYER_ID)
    {
        const float numberCenterX = (barX0 + barX1) * 0.5f;
        float x = numberCenterX - numberWidth * 0.5f;
        const float digitY0 = (barY0 + barY1) * 0.5f - kDigitScreenH * 0.5f;

        std::vector<gfx::TexturedQuad> quads;
        quads.reserve(static_cast<std::size_t>(digitCount));

        appendGlyphRun(quads, digits.data(), digitCount, x, digitY0, kDigitScreenH, metrics.aspect, 1.0f, kGap);

        mVulkan.updateTexturedLayer(mScoreLayerId, quads);
    }
}

void App::updateStageLabelRenderData()
{
    if (mStageLabelLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
    {
        return;
    }
    if (mGamePhase == GamePhase::ResultsScreen || mGamePhase == GamePhase::GameVictory)
    {
        mVulkan.clearTexturedLayer(mStageLabelLayerId);
        return;
    }

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        mVulkan.clearTexturedLayer(mStageLabelLayerId);
        return;
    }

    const HudLayout hudLayout = makeHudLayout();
    constexpr float kCharScreenH = 0.050f;
    constexpr float kGap = 0.005f;

    const auto &config = stage::kStageTable[mStageState.currentStageIndex];
    const auto &extConfig = kExtendedStageTable[mStageState.currentStageIndex];

    std::array<UiGlyph, 4> stageDigits = {};
    const int stageDigitCount = writeNumberRun(config.stageNumber, stageDigits);
    constexpr UiGlyph kWordStage[] = {UiGlyph::WordStage};

    // رمز نوع المرحلة
    UiGlyph typeGlyph = UiGlyph::Minus;
    bool hasTypeGlyph = false;
    float typeAlpha = 0.72f;
    switch (extConfig.type)
    {
    case StageType::Bonus:     typeGlyph = UiGlyph::WordBonus;     typeAlpha = 0.86f; hasTypeGlyph = true; break;
    case StageType::Challenge: typeGlyph = UiGlyph::WordChallenge; typeAlpha = 0.82f; hasTypeGlyph = true; break;
    case StageType::Accuracy:  typeGlyph = UiGlyph::WordAccuracy;  typeAlpha = 0.78f; hasTypeGlyph = true; break;
    case StageType::Boss:      typeGlyph = UiGlyph::WordBoss;      typeAlpha = 0.94f; hasTypeGlyph = true; break;
    case StageType::Rare:      typeGlyph = UiGlyph::WordRare;      typeAlpha = 0.88f; hasTypeGlyph = true; break;
    default: break;
    }

    const float digitsW = measureGlyphRun(stageDigits.data(), stageDigitCount, kCharScreenH, metrics.aspect, kGap);
    const float wordW = measureGlyphRun(kWordStage, 1, kCharScreenH, metrics.aspect, kGap);
    const float spaceW = ui::text::glyphScreenWidth(UiGlyph::Digit0, kCharScreenH, metrics.aspect) * 0.5f;
    const float typeGapW = hasTypeGlyph
                               ? (ui::text::glyphScreenWidth(UiGlyph::Digit0, kCharScreenH, metrics.aspect) * 0.3f)
                               : 0.0f;
    const float typeW = hasTypeGlyph
                            ? measureGlyphRun(&typeGlyph, 1, kCharScreenH, metrics.aspect, kGap)
                            : 0.0f;
    const float totalW = digitsW + spaceW + wordW + typeGapW + typeW;

    /*
     * نثبت عنوان المرحلة يسار شريط التقدم بدل مزاحمته لعداد الطلقات.
     * anchorRightX يمثل الحافة اليمنى للعبارة ضمن مساحة HUD اليسرى.
     */
    float x = hudLayout.stageRightX - totalW;
    const float y0 = hudLayout.primaryRowY0;

    std::vector<gfx::TexturedQuad> quads;
    quads.reserve(static_cast<std::size_t>(stageDigitCount + 2));

    // 1) رقم المرحلة (يسار)
    appendGlyphRun(quads, stageDigits.data(), stageDigitCount, x, y0, kCharScreenH, metrics.aspect, 1.0f, kGap);

    // فراغ
    x += spaceW - kGap;

    // 2) الكلمة العربية ترسم ككتلة واحدة بعد تشكيلها داخل الأطلس.
    appendGlyphRun(quads, kWordStage, 1, x, y0, kCharScreenH, metrics.aspect, 0.85f, kGap);

    // 3) رمز النوع
    if (hasTypeGlyph)
    {
        x += ui::text::glyphScreenWidth(UiGlyph::Digit0, kCharScreenH, metrics.aspect) * 0.3f - kGap;
        ui::text::appendGlyphQuad(quads, typeGlyph, x, y0, kCharScreenH, metrics.aspect, typeAlpha, kGap);
    }

    mVulkan.updateTexturedLayer(mStageLabelLayerId, quads);
}

void App::updateShotsDisplayRenderData()
{
    if (mShotsDisplayLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
    {
        return;
    }
    if (mGamePhase == GamePhase::ResultsScreen || mGamePhase == GamePhase::GameVictory)
    {
        mVulkan.clearTexturedLayer(mShotsDisplayLayerId);
        return;
    }

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        mVulkan.clearTexturedLayer(mShotsDisplayLayerId);
        return;
    }

    const HudLayout hudLayout = makeHudLayout();
    constexpr float kWordScreenH = 0.043f;
    constexpr float kValueScreenH = 0.048f;
    constexpr float kGap = 0.005f;

    const int shotsRemaining = mStageState.shotsRemaining;
    const int shotsTotal = mStageState.initialShots;

    std::array<UiGlyph, 12> chars = {};
    const int charCount = writeFractionRun(shotsRemaining, shotsTotal, chars);

    std::vector<gfx::TexturedQuad> quads;
    quads.reserve(static_cast<std::size_t>(charCount + 1));
    appendRightAnchoredArabicWithNumbers(quads, UiGlyph::WordShots, chars.data(), charCount, hudLayout.statsRightX,
                                         hudLayout.primaryRowY0, kWordScreenH, kValueScreenH, metrics.aspect, 0.72f,
                                         1.0f, kGap);

    mVulkan.updateTexturedLayer(mShotsDisplayLayerId, quads);
}

void App::updateDucksRemainingRenderData()
{
    if (mDucksRemainingLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
    {
        return;
    }
    if (mGamePhase == GamePhase::ResultsScreen || mGamePhase == GamePhase::GameVictory ||
        mGamePhase == GamePhase::StageIntro)
    {
        mVulkan.clearTexturedLayer(mDucksRemainingLayerId);
        return;
    }

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        mVulkan.clearTexturedLayer(mDucksRemainingLayerId);
        return;
    }

    const HudLayout hudLayout = makeHudLayout();
    constexpr float kWordScreenH = 0.043f;
    constexpr float kValueScreenH = 0.048f;
    constexpr float kGap = 0.005f;

    const auto &stageConfig = stage::kStageTable[mStageState.currentStageIndex];
    const int ducksRemaining = std::max(stageConfig.totalDucks - mStageState.ducksHitThisStage, 0);

    std::array<UiGlyph, 4> remainingDigits = {};
    const int remainingCount = writeNumberRun(ducksRemaining, remainingDigits);

    std::vector<gfx::TexturedQuad> quads;
    quads.reserve(static_cast<std::size_t>(remainingCount + 1));
    appendRightAnchoredArabicWithNumbers(quads, UiGlyph::WordDucks, remainingDigits.data(), remainingCount,
                                         hudLayout.ducksRightX, hudLayout.primaryRowY0, kWordScreenH,
                                         kValueScreenH, metrics.aspect, 0.74f, 0.92f, kGap);

    mVulkan.updateTexturedLayer(mDucksRemainingLayerId, quads);
}

void App::updateResultsRenderData()
{
    // شاشة النتائج
    const bool showResults = mGamePhase == GamePhase::ResultsScreen || mGamePhase == GamePhase::GameVictory;
    if (!showResults)
    {
        if (mResultsBgLayerId != gfx::VulkanContext::INVALID_LAYER_ID)
        {
            mVulkan.clearTexturedLayer(mResultsBgLayerId);
        }
        if (mResultsContentLayerId != gfx::VulkanContext::INVALID_LAYER_ID)
        {
            mVulkan.clearTexturedLayer(mResultsContentLayerId);
        }
        return;
    }

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        if (mResultsBgLayerId != gfx::VulkanContext::INVALID_LAYER_ID)
        {
            mVulkan.clearTexturedLayer(mResultsBgLayerId);
        }
        if (mResultsContentLayerId != gfx::VulkanContext::INVALID_LAYER_ID)
        {
            mVulkan.clearTexturedLayer(mResultsContentLayerId);
        }
        return;
    }

    const bool hasBonusLine = mGamePhase == GamePhase::ResultsScreen && mLastStageResult.passed &&
                              mLastReward.bonusScore > 0;
    const ResultsLayout resultsLayout = makeResultsLayout(hasBonusLine);

    // لوحة الخلفية
    if (mResultsBgLayerId != gfx::VulkanContext::INVALID_LAYER_ID)
    {
        const gfx::TexturedQuad bgQuad =
            gfx::makeTexturedQuad(gfx::makeScreenRect(resultsLayout.panelX0, resultsLayout.panelX1, resultsLayout.panelY0,
                                                      resultsLayout.panelY1),
                                  gfx::fullUvRect(), 1.0f);
        mVulkan.updateTexturedLayer(mResultsBgLayerId, bgQuad);
    }

    // محتوى النتائج
    if (mResultsContentLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
    {
        return;
    }

    constexpr float kTitleCharH = 0.066f;
    constexpr float kStatusCharH = 0.058f;
    constexpr float kLabelCharH = 0.045f;
    constexpr float kValueCharH = 0.047f;
    constexpr float kGap = 0.006f;
    std::vector<gfx::TexturedQuad> quads;
    constexpr float kFooterCharH = 0.043f;

    const auto appendCenteredRun = [&](const UiGlyph* glyphs, int count, float centerX, float y0, float height,
                                       float alpha) {
        float x = centerX - measureGlyphRun(glyphs, count, height, metrics.aspect, kGap) * 0.5f;
        appendGlyphRun(quads, glyphs, count, x, y0, height, metrics.aspect, alpha, kGap);
    };

    if (mGamePhase == GamePhase::GameVictory)
    {
        // ===== شاشة الفوز النهائي =====
        const UiGlyph victoryWord[] = {UiGlyph::WordVictory};
        appendCenteredRun(victoryWord, 1, 0.0f, resultsLayout.titleY0, kTitleCharH, 1.0f);

        // سطر ثانوي
        const UiGlyph passWord[] = {UiGlyph::WordPass};
        appendCenteredRun(passWord, 1, 0.0f, resultsLayout.subtitleY0, kStatusCharH, 0.82f);

        // التذييل
        const UiGlyph continueWord[] = {UiGlyph::WordContinue};
        appendCenteredRun(continueWord, 1, 0.0f, resultsLayout.footerY0, kFooterCharH, 0.58f);
    }
    else
    {
        // ===== شاشة نتائج المرحلة =====
        const UiGlyph statusWord[] = {mLastStageResult.passed ? UiGlyph::WordPass : UiGlyph::WordFail};
        appendCenteredRun(statusWord, 1, 0.0f, resultsLayout.titleY0, kStatusCharH, 1.0f);

        // السطر الأول: مرحلة XX
        {
            std::array<UiGlyph, 4> stageDigits = {};
            const int stageCount = writeNumberRun(mLastStageResult.stageNumber, stageDigits);
            appendMetricRow(quads, UiGlyph::WordStage, stageDigits.data(), stageCount, resultsLayout.labelRightX,
                            resultsLayout.valueRightX, resultsLayout.metricsStartY, kLabelCharH, kValueCharH,
                            metrics.aspect, 0.74f, 1.0f, kGap);
        }

        // السطر الثاني: إصابات N/M
        {
            std::array<UiGlyph, 12> nums = {};
            const int nc = writeFractionRun(mLastStageResult.ducksHit, mLastStageResult.ducksTotal, nums);
            appendMetricRow(quads, UiGlyph::WordHits, nums.data(), nc, resultsLayout.labelRightX,
                            resultsLayout.valueRightX, resultsLayout.metricsStartY + resultsLayout.metricGap,
                            kLabelCharH, kValueCharH, metrics.aspect, 0.74f, 1.0f, kGap);
        }

        // السطر الثالث: الطلقات المتبقية / الإجمالي
        {
            std::array<UiGlyph, 12> nums = {};
            const int nc = writeFractionRun(mLastStageResult.shotsRemaining, mLastStageResult.shotsTotal, nums);
            appendMetricRow(quads, UiGlyph::WordShots, nums.data(), nc, resultsLayout.labelRightX,
                            resultsLayout.valueRightX, resultsLayout.metricsStartY + resultsLayout.metricGap * 2.0f,
                            kLabelCharH, kValueCharH, metrics.aspect, 0.74f, 0.88f, kGap);
        }

        // السطر الرابع: دقة XX%
        {
            const int accPct = static_cast<int>(mLastStageResult.accuracy * 100.0f);
            std::array<UiGlyph, 4> accDigits = {};
            const int accCount = writeNumberRun(accPct, accDigits);

            std::array<UiGlyph, 6> nums = {};
            int nc = 0;
            for (int i = 0; i < accCount; ++i) { nums[nc++] = accDigits[static_cast<std::size_t>(i)]; }
            nums[nc++] = UiGlyph::Percent;
            appendMetricRow(quads, UiGlyph::WordAccuracy, nums.data(), nc, resultsLayout.labelRightX,
                            resultsLayout.valueRightX, resultsLayout.metricsStartY + resultsLayout.metricGap * 3.0f,
                            kLabelCharH, kValueCharH, metrics.aspect, 0.70f, 0.82f, kGap);
        }

        // سطر المكافأة
        if (hasBonusLine)
        {
            std::array<UiGlyph, 8> scoreDigits = {};
            const int scoreCount = writeNumberRun(mLastReward.bonusScore, scoreDigits);

            std::array<UiGlyph, 10> nums = {};
            int nc = 0;
            nums[nc++] = UiGlyph::Plus;
            for (int i = 0; i < scoreCount; ++i) { nums[nc++] = scoreDigits[static_cast<std::size_t>(i)]; }
            appendMetricRow(quads, UiGlyph::WordPoints, nums.data(), nc, resultsLayout.labelRightX,
                            resultsLayout.valueRightX, resultsLayout.metricsStartY + resultsLayout.metricGap * 4.0f,
                            kLabelCharH, kValueCharH, metrics.aspect, 0.82f, 0.96f, kGap);
        }

        // التذييل
        const UiGlyph continueWord[] = {UiGlyph::WordContinue};
        appendCenteredRun(continueWord, 1, 0.0f, resultsLayout.footerY0, kFooterCharH, 0.56f);
    }

    mVulkan.updateTexturedLayer(mResultsContentLayerId, quads);
}

void App::updateGroundInteraction(float deltaTime)
{
    mLeftGroundPressure = std::max(0.0f, mLeftGroundPressure - deltaTime * kFootprintDecayPerSecond);
    mRightGroundPressure = std::max(0.0f, mRightGroundPressure - deltaTime * kFootprintDecayPerSecond);

    const bool walking = hunterplay::isWalkingClip(mHunterState);

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    const game::AtlasFrame* frame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
    if (walking && metrics.valid() && frame != nullptr && frame->sourceW > 0 && frame->sourceH > 0)
    {
        const float logicalHalfWidth = scene::hunterLogicalHalfWidth(*frame, metrics);
        const float hunterWidth = logicalHalfWidth * 2.0f;
        const float gait = scene::resolveGait(mHunterState.elapsed);
        const float leftTargetPressure = scene::resolvePressure(gait, true);
        const float rightTargetPressure = scene::resolvePressure(gait, false);
        const float leftFootTargetX = scene::resolveFootTargetX(mHunter.x, hunterWidth, gait, true);
        const float rightFootTargetX = scene::resolveFootTargetX(mHunter.x, hunterWidth, gait, false);
        const float followFactor = std::clamp(deltaTime * kFootprintFollowPerSecond, 0.0f, 1.0f);

        mGroundFootRadius = std::max(hunterWidth * 0.16f, 0.05f);

        if (leftTargetPressure > kFootContactThreshold)
        {
            if (mLeftGroundPressure <= kFootContactThreshold)
            {
                mLeftGroundX = leftFootTargetX;
            }
            mLeftGroundX += (leftFootTargetX - mLeftGroundX) * followFactor;
            mLeftGroundPressure = std::max(mLeftGroundPressure, leftTargetPressure * leftTargetPressure);
        }

        if (rightTargetPressure > kFootContactThreshold)
        {
            if (mRightGroundPressure <= kFootContactThreshold)
            {
                mRightGroundX = rightFootTargetX;
            }
            mRightGroundX += (rightFootTargetX - mRightGroundX) * followFactor;
            mRightGroundPressure = std::max(mRightGroundPressure, rightTargetPressure * rightTargetPressure);
        }
    }

    mVulkan.setGroundInteraction(mLeftGroundX, mRightGroundX, mGroundFootRadius, mLeftGroundPressure,
                                 mRightGroundPressure);
}

void App::render()
{
    mVulkan.drawFrame();
}

} // namespace core
