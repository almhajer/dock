#include "App.h"
#include "SceneLayout.h"
#include "../ui/TextAtlas.h"
#include "../game/HunterSpriteAtlas.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace core
{

/*
 * هذا الملف مسؤول عن دورة حياة التطبيق:
 * - التهيئة الأولية
 * - تشغيل الحلقة الرئيسية
 * - الإغلاق والتنظيف
 */

App::App(const Config& config)
    : mWindow(config.window), mDuckRandomEngine(std::random_device{}()), mAssetsPath(config.assetsPath)
{
    init();
}

App::~App()
{
    cleanup();
}

void App::init()
{
    if (mInitialized)
    {
        return;
    }

    mInput.init(mWindow.getHandle());

    mVulkan.init(mWindow.getHandle(), mWindow.getWidth(), mWindow.getHeight());

    constexpr std::array<unsigned char, 4> WHITE_PIXEL = {255, 255, 255, 255};
    mSoilLayerId = mVulkan.createTexturedLayerFromPixels(WHITE_PIXEL.data(), 1, 1, 1);

    /*
     * أطلس الصياد يجمع الحركة والإطلاقين داخل ملف واحد،
     * لذلك نربطه هنا مرة واحدة ثم نترك التبديل للأنيميشن لاحقاً.
     */
    const std::string atlasPath = mAssetsPath + "/sprite/hunter_atlas.png";
    mHunterLayerId =
        mVulkan.createTexturedLayerWithCallback(atlasPath, 1, [this](int w, int h, const unsigned char* /*pixels*/)
                                                { mSpriteAnim.setAtlasData(game::createHunterAtlasData(w, h)); });

    mHunterShotSound.load(mAssetsPath + "/audio/hunter_shot.mp3");
    mDuckAmbientSound.load(mAssetsPath + "/audio/douck.wav");

    mGrassLayerId = mVulkan.createTexturedLayerFromPixels(WHITE_PIXEL.data(), 1, 1, scene::kMaxGrassQuads);
    std::cout << "[Grass] تم تفعيل العشب الإجرائي عبر الشيدر" << std::endl;

    const std::string duckSourcePath = mAssetsPath + "/sprite/duck.png";
    const game::DuckAtlasSheet duckSheet = game::loadDuckAtlasSheetFromSourceImage(duckSourcePath);
    mDuckAnim.setAtlasData(duckSheet.atlas);
    mDuckLayerId =
        mVulkan.createTexturedLayerFromPixels(duckSheet.pixels.data(), duckSheet.imageWidth, duckSheet.imageHeight,
                                               kMaxSimultaneousDucks);

    {
        const auto generateRoundedRect = [](int width, int height, int radius, float maxAlpha,
                                            unsigned char r, unsigned char g,
                                            unsigned char b) -> std::vector<unsigned char> {
            std::vector<unsigned char> pixels(static_cast<std::size_t>(width * height * 4), 0);
            const float halfW = static_cast<float>(width) * 0.5f;
            const float halfH = static_cast<float>(height) * 0.5f;
            const float rad = static_cast<float>(radius);
            const float maxA = maxAlpha / 255.0f;
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    const float px = static_cast<float>(x) + 0.5f - halfW;
                    const float py = static_cast<float>(y) + 0.5f - halfH;
                    const float dx = std::abs(px) - halfW + rad;
                    const float dy = std::abs(py) - halfH + rad;
                    const float outsideDist = std::sqrt(std::max(dx, 0.0f) * std::max(dx, 0.0f) +
                                                        std::max(dy, 0.0f) * std::max(dy, 0.0f));
                    const float insideDist = std::min(std::max(dx, dy), 0.0f);
                    const float dist = outsideDist + insideDist - rad;
                    const float alpha = std::clamp(0.5f - dist, 0.0f, 1.0f) * maxA;
                    const int idx = (y * width + x) * 4;
                    pixels[idx + 0] = r;
                    pixels[idx + 1] = g;
                    pixels[idx + 2] = b;
                    pixels[idx + 3] = static_cast<unsigned char>(alpha * 255.0f);
                }
            }
            return pixels;
        };

        constexpr int kRoundedW = 192;
        constexpr int kRoundedH = 28;
        constexpr int kRoundedR = 14;

        auto bgPixels = generateRoundedRect(kRoundedW, kRoundedH, kRoundedR, 104.0f, 12, 14, 24);
        mScoreBgLayerId = mVulkan.createTexturedLayerFromPixels(bgPixels.data(), kRoundedW, kRoundedH, 1);

        auto fillPixels = generateRoundedRect(kRoundedW, kRoundedH, kRoundedR, 232.0f, 255, 201, 52);
        mScoreFillLayerId = mVulkan.createTexturedLayerFromPixels(fillPixels.data(), kRoundedW, kRoundedH, 1);
    }

    {
        const ui::text::TextAtlas& textAtlas = ui::text::atlas();
        mScoreLayerId =
            mVulkan.createTexturedLayerFromPixels(textAtlas.pixels.data(), textAtlas.width, textAtlas.height, 32);
        mStageLabelLayerId =
            mVulkan.createTexturedLayerFromPixels(textAtlas.pixels.data(), textAtlas.width, textAtlas.height, 16);
        mShotsDisplayLayerId =
            mVulkan.createTexturedLayerFromPixels(textAtlas.pixels.data(), textAtlas.width, textAtlas.height, 16);
        mDucksRemainingLayerId =
            mVulkan.createTexturedLayerFromPixels(textAtlas.pixels.data(), textAtlas.width, textAtlas.height, 8);
        mResultsContentLayerId =
            mVulkan.createTexturedLayerFromPixels(textAtlas.pixels.data(), textAtlas.width, textAtlas.height, 64);
    }

    {
        const auto generateRoundedRect = [](int width, int height, int radius, float maxAlpha,
                                            unsigned char r, unsigned char g,
                                            unsigned char b) -> std::vector<unsigned char> {
            std::vector<unsigned char> pixels(static_cast<std::size_t>(width * height * 4), 0);
            const float halfW = static_cast<float>(width) * 0.5f;
            const float halfH = static_cast<float>(height) * 0.5f;
            const float rad = static_cast<float>(radius);
            const float maxA = maxAlpha / 255.0f;
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    const float px = static_cast<float>(x) + 0.5f - halfW;
                    const float py = static_cast<float>(y) + 0.5f - halfH;
                    const float ddx = std::abs(px) - halfW + rad;
                    const float ddy = std::abs(py) - halfH + rad;
                    const float outsideDist = std::sqrt(std::max(ddx, 0.0f) * std::max(ddx, 0.0f) +
                                                        std::max(ddy, 0.0f) * std::max(ddy, 0.0f));
                    const float insideDist = std::min(std::max(ddx, ddy), 0.0f);
                    const float dist = outsideDist + insideDist - rad;
                    const float alpha = std::clamp(0.5f - dist, 0.0f, 1.0f) * maxA;
                    const int idx = (y * width + x) * 4;
                    pixels[idx + 0] = r;
                    pixels[idx + 1] = g;
                    pixels[idx + 2] = b;
                    pixels[idx + 3] = static_cast<unsigned char>(alpha * 255.0f);
                }
            }
            return pixels;
        };

        constexpr int kResultsBgW = 320;
        constexpr int kResultsBgH = 256;
        constexpr int kResultsBgR = 26;
        auto resultsBg = generateRoundedRect(kResultsBgW, kResultsBgH, kResultsBgR, 205.0f, 14, 16, 30);
        mResultsBgLayerId = mVulkan.createTexturedLayerFromPixels(resultsBg.data(), kResultsBgW, kResultsBgH, 1);
    }

    mSpriteAnim.play(mHunterState, "idle_right");
    mDuckPool.initialize(&mDuckAnim, &mDuckRandomEngine);
    mNatureSystem.initialize(scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight()));

    mGamePhase = GamePhase::StageIntro;
    startStage(0);

    mInitialized = true;
    mRunning = true;
}

void App::run()
{
    while (mRunning && mWindow.isOpen())
    {
        mTimer.update();
        mInput.update();
        mWindow.pollEvents();

        if (mWindow.wasResized())
        {
            mVulkan.onResize(mWindow.getWidth(), mWindow.getHeight());
        }

        update(mTimer.getDeltaTime());
        render();
    }

    mRunning = false;
}

void App::requestShutdown()
{
    mRunning = false;
}

bool App::isRunning() const
{
    return mRunning;
}

Window& App::getWindow()
{
    return mWindow;
}
const Window& App::getWindow() const
{
    return mWindow;
}
Input& App::getInput()
{
    return mInput;
}
const Input& App::getInput() const
{
    return mInput;
}
Timer& App::getTimer()
{
    return mTimer;
}
const Timer& App::getTimer() const
{
    return mTimer;
}

void App::cleanup()
{
    if (!mInitialized)
    {
        return;
    }

    mInput.shutdown();
    mHunterShotSound.reset();
    mDuckAmbientSound.reset();

    mVulkan.waitIdle();
    mVulkan.cleanup();
    mInitialized = false;
}

} // namespace core
