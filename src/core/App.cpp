#include "App.h"
#include "SceneLayout.h"
#include "../game/HunterSpriteAtlas.h"

#include <array>
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
    : mWindow(config.window), mAssetsPath(config.assetsPath), mDuckRandomEngine(std::random_device{}())
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
        mVulkan.createTexturedLayerFromPixels(duckSheet.pixels.data(), duckSheet.imageWidth, duckSheet.imageHeight, 1);

    mSpriteAnim.play(mHunterState, "idle_right");
    mDuckAnim.play(mDuckState, "fly_right");
    resetDuckFlight();
    mNatureSystem.initialize(scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight()));

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
