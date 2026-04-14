#pragma once

#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "StageDefinitions.h"
#include "StageObjectives.h"
#include "DuckPool.h"
#include "WaveManager.h"
#include "SkillAssessment.h"
#include "RewardSystem.h"
#include "../audio/SoundEffectPlayer.h"
#include "../game/DuckSpriteAtlas.h"
#include "../game/NatureSystem.h"
#include "../gfx/VulkanContext.h"
#include "../game/SpriteAnimation.h"

#include <random>
#include <string>

namespace core
{

/*
 حلقة التطبيق الرئيسية - تنسق بين النافذة والإدخال واللعبة
 */
class App
{
  public:
#pragma region PublicTypes
    /*
     إعدادات التطبيق
     */
    struct Config
    {
        /*
         إعدادات نافذة التطبيق.
         */
        Window::Config window; // إعدادات النافذة

        /*
         مسار مجلد الأصول الأساسي.
         */
        std::string assetsPath; // مسار مجلد الأصول

        Config() : assetsPath("assets")
        {
        }
    };
#pragma endregion PublicTypes

    /*
     ينشئ التطبيق ويبدأ تهيئة أنظمته.
     */
    explicit App(const Config& config = {});

    /*
     يحرر الموارد المملوكة للتطبيق.
     */
    ~App();

    // منع النسخ
    App(const App&) = delete;
    App& operator=(const App&) = delete;

#pragma region PublicLifecycle
    /*
     بدء حلقة التطبيق (تستمر حتى إغلاق النافذة)
     */
    void run();

    /*
     طلب إيقاف التطبيق
     */
    void requestShutdown();

    /*
     هل التطبيق يعمل؟
     */
    [[nodiscard]] bool isRunning() const;

    /*
     الوصول لمكونات التطبيق
     */
    [[nodiscard]] Window& getWindow();
    [[nodiscard]] const Window& getWindow() const;
    [[nodiscard]] Input& getInput();
    [[nodiscard]] const Input& getInput() const;
    [[nodiscard]] Timer& getTimer();
    [[nodiscard]] const Timer& getTimer() const;

#pragma endregion PublicLifecycle

  private:
#pragma region PrivateTypes
    /*
     مرحلة حركة الصياد الحالية
     */
    enum class HunterPhase : unsigned char
    {
        Locomotion, // مشي أو وقوف
        Shoot,      // تسلسل الإطلاق العادي
        ShootHigh,  // تسلسل الإطلاق العالي
    };

    /*
     مرحلة اللعبة الكلية
     */
    enum class GamePhase : unsigned char
    {
        StageIntro,    // عرض رقم المرحلة قبل بدء اللعب
        Playing,       // اللعب الفعلي داخل المرحلة
        StageComplete, // المرحلة انتهت بنجاح
        StageFailed,   // المرحلة انتهت بالفشل
        ResultsScreen, // شاشة النتائج التفصيلية
        GameVictory,   // جميع المراحل اكتملت
    };
#pragma endregion PrivateTypes

#pragma region PrivateLifecycle
    void init();
    void update(float deltaTime);
    void updateHunterMotion(float deltaTime);
    void startStage(int stageIndex);
    void endStage(bool won);
    void updateNatureSystem(float deltaTime);
    void updateGrassRenderData();
    void updateSoilRenderData();
    void updateHunterRenderData();
    void updateDuckRenderData();
    void updateScoreRenderData();
    void updateStageLabelRenderData();
    void updateShotsDisplayRenderData();
    void updateDucksRemainingRenderData();
    void updateResultsRenderData();
    void updateNatureRenderData();
    void updateGroundInteraction(float deltaTime);
    void render();
    void cleanup();
#pragma endregion PrivateLifecycle

#pragma region CoreComponents
    Window mWindow;
    Input mInput;
    Timer mTimer;
    gfx::VulkanContext mVulkan;
    audio::SoundEffectPlayer mHunterShotSound;
    audio::SoundEffectPlayer mDuckAmbientSound;
    game::NatureSystem mNatureSystem;
    gfx::EnvironmentRenderData mEnvironmentRenderData;
#pragma endregion CoreComponents

#pragma region DuckSystems
    /*
     أنظمة البط والمراحل الجديدة
     */
    game::SpriteAnimation mDuckAnim;
    DuckPool mDuckPool;
    WaveManager mWaveManager;
    SkillAssessment mSkillAssessment;
    RewardSystem mRewardSystem;
    std::mt19937 mDuckRandomEngine;
#pragma endregion DuckSystems

#pragma region AnimationState
    game::SpriteAnimation mSpriteAnim;
    game::AnimationState mHunterState;
    HunterPhase mHunterPhase = HunterPhase::Locomotion;
    std::string mAssetsPath;
#pragma endregion AnimationState

#pragma region RenderLayerIds
    gfx::VulkanContext::LayerId mGrassLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mHunterLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mDuckLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mSoilLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mScoreBgLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mScoreFillLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mScoreLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mStageLabelLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mShotsDisplayLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mDucksRemainingLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mResultsBgLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mResultsContentLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
#pragma endregion RenderLayerIds

#pragma region LayoutCache
    uint32_t mGrassLayoutWidth = 0;
    uint32_t mGrassLayoutHeight = 0;
    uint32_t mSoilLayoutWidth = 0;
    uint32_t mSoilLayoutHeight = 0;
#pragma endregion LayoutCache

#pragma region GroundInteractionState
    float mLeftGroundX = 0.0f;
    float mRightGroundX = 0.0f;
    float mLeftGroundPressure = 0.0f;
    float mRightGroundPressure = 0.0f;
    float mGroundFootRadius = 0.06f;
#pragma endregion GroundInteractionState

#pragma region HunterState
    float mHunterX = 0.0f;
    float mHunterSpeed = 0.8f;
    float mReloadTimer = 0.0f;
    float mHunterShotRecoveryTimer = 0.0f;
    float mShotCooldown = 0.0f;
    bool mFacingLeft = false;
    bool mIsReloading = false;
#pragma endregion HunterState

#pragma region GameState
    int mScore = 0;
    GamePhase mGamePhase = GamePhase::Playing;
    stage::StageState mStageState;
    stage::StageResult mLastStageResult;
    StageReward mLastReward;
    float mPhaseTimer = 0.0f;
    bool mRunning = false;
    bool mInitialized = false;
    float mDifficultyMultiplier = 1.0f;
#pragma endregion GameState
};

} // namespace core
