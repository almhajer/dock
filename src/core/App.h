#pragma once

#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "StageDefinitions.h"
#include "StageObjectives.h"
#include "DuckPool.h"
#include "HunterController.h"
#include "../gameplay/WaveManager.h"
#include "../gameplay/SkillAssessment.h"
#include "../gameplay/RewardSystem.h"
#include "AsmaulHusnaOverlay.h"
#include "../audio/SoundEffectPlayer.h"
#include "../assets/DuckSpriteAtlas.h"
#include "../assets/NatureSystem.h"
#include "../gfx/VulkanContext.h"
#include "../assets/SpriteAnimation.h"

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

    /**
     * @brief ينشئ التطبيق ويبدأ تهيئة أنظمته.
     * @param config إعدادات التطبيق المطلوبة.
     * @throws std::runtime_error إذا فشلت التهيئة.
     */
    explicit App(const Config& config = {});

    /**
     * @brief يحرر الموارد المملوكة للتطبيق.
     * @remarks يتم استدعاؤه تلقائيًا عند انتهاء عمر الكائن.
     */
    ~App();

    // منع النسخ
    App(const App&) = delete;
    App& operator=(const App&) = delete;

#pragma region PublicLifecycle
    /**
     * @brief بدء حلقة التطبيق (تستمر حتى إغلاق النافذة).
     * @remarks تحتوي على حلقة رئيسية تتعامل مع الإدخال، التحديث، والرسم.
     */
    void run();

    /**
     * @brief طلب إيقاف التطبيق.
     * @remarks يضع علامة لإنهاء الحلقة الرئيسية في الدورة التالية.
     */
    void requestShutdown();

    /**
     * @brief هل التطبيق يعمل حاليًا؟
     * @return true إذا كانت الحلقة الرئيسية مستمرة، false خلاف ذلك.
     */
    [[nodiscard]] bool isRunning() const;

    /**
     * @brief الوصول لمكونات التطبيق.
     * @return مرجع للنافذة.
     */
    [[nodiscard]] Window& getWindow();
    /**
     * @brief الوصول لمكونات التطبيق (نسخة ثابتة).
     * @return مرجع ثابت للنافذة.
     */
    [[nodiscard]] const Window& getWindow() const;
    /**
     * @brief الوصول لمكونات التطبيق.
     * @return مرجع للإدخال.
     */
    [[nodiscard]] Input& getInput();
    /**
     * @brief الوصول لمكونات التطبيق (نسخة ثابتة).
     * @return مرجع ثابت للإدخال.
     */
    [[nodiscard]] const Input& getInput() const;
    /**
     * @brief الوصول لمكونات التطبيق.
     * @return مرجع للمؤقت.
     */
    [[nodiscard]] Timer& getTimer();
    /**
     * @brief الوصول لمكونات التطبيق (نسخة ثابتة).
     * @return مرجع ثابت للمؤقت.
     */
    [[nodiscard]] const Timer& getTimer() const;

#pragma endregion PublicLifecycle

  private:
#pragma region PrivateTypes
    /*
     مرحلة اللعبة الكلية
     */
    enum class GamePhase : unsigned char
    {
        Intro,         // شاشة الافتتاحية قبل بدء اللعب
        StageIntro,    // عرض رقم المرحلة قبل بدء اللعب
        Playing,       // اللعب الفعلي داخل المرحلة
        StageComplete, // المرحلة انتهت بنجاح
        StageFailed,   // المرحلة انتهت بالفشل
        ResultsScreen, // شاشة النتائج التفصيلية
        GameVictory,   // جميع المراحل اكتملت
        Paused,        // إيقاف مؤقت
    };
#pragma endregion PrivateTypes

#pragma region PrivateLifecycle
    void init();
    void update(float deltaTime);
    void updateAllRenderData(float deltaTime, const scene::WindowMetrics& metrics);
    void updateHunterMotion(float deltaTime);
    void enterHunterStageEndPose();
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
    void updateAsmaOverlayRenderData();
    void updatePauseRenderData();
    void render();
    void cleanup();
    void saveGameState();
    bool loadGameState();
    std::string saveFilePath() const;
#pragma endregion PrivateLifecycle

#pragma region CoreComponents
    Window mWindow;
    Input mInput;
    Timer mTimer;
    gfx::VulkanContext mVulkan;
    audio::SoundEffectPlayer mHunterShotSound;
    audio::SoundEffectPlayer mDuckAmbientSound;
    audio::SoundEffectPlayer mIntroSound;
    AsmaulHusnaOverlay mAsmaOverlay;
    audio::SoundEffectPlayer mAsmaAudio;
    audio::SoundEffectPlayer mAsmaAudioNext;
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
    huntercontroller::State mHunter;
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
    gfx::VulkanContext::LayerId mAsmaTextLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mAsmaBgLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
    gfx::VulkanContext::LayerId mPauseLayerId = gfx::VulkanContext::INVALID_LAYER_ID;
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

#pragma region GameState
    int mScore = 0;
    GamePhase mGamePhase = GamePhase::Playing;
    GamePhase mPhaseBeforePause = GamePhase::Playing;
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
