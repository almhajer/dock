#include "App.h"
#include "DuckGameplay.h"
#include "HunterGameplay.h"
#include "SceneLayout.h"
#include "../game/HunterSpriteData.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

namespace core
{
    namespace hc = huntercontroller;

    /*
     * هذا الملف مسؤول عن منطق اللعب:
     * - تحديث حالة الصياد
     * - تحديث البطات المتعددة عبر DuckPool
     * - إدارة الدفعات عبر WaveManager
     * - كشف الإصابة والتحويل بين الحالات
     */

    void App::update(float deltaTime)
    {
        updateNatureSystem(deltaTime);

        const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());

        // تحديث منطق المراحل
        switch (mGamePhase)
        {
        case GamePhase::StageIntro:
            mPhaseTimer -= deltaTime;
            updateHunterMotion(deltaTime);
            if (mPhaseTimer <= 0.0f)
            {
                mGamePhase = GamePhase::Playing;
                mStageState.stageActive = true;
            }
            break;

        case GamePhase::Playing:
        {
            mStageState.stageElapsedTime += deltaTime;
            mSkillAssessment.update(deltaTime);

            // قراءة العدادات قبل أي تغيير (tryHit يحدث داخل updateHunterMotion)
            const int prevHit = mDuckPool.totalHitCount();
            const int prevDespawned = mDuckPool.totalDespawnedCount();

            updateHunterMotion(deltaTime);
            mDuckPool.update(deltaTime);

            // تسجيل التغيرات
            const int newHits = mDuckPool.totalHitCount() - prevHit;
            const int newDespawned = mDuckPool.totalDespawnedCount() - prevDespawned;
            for (int i = 0; i < newHits; ++i)
            {
                ++mStageState.ducksHitThisStage;
                ++mScore;
                mWaveManager.onDuckHit();
            }
            for (int i = 0; i < newDespawned; ++i)
            {
                mWaveManager.onDuckDespawned();
                mSkillAssessment.onDuckEscaped();
            }

            // تحديث الدفعات
            const auto& config = stage::kStageTable[mStageState.currentStageIndex];
            DuckSpawnContext spawnCtx;
            /*
             * كل مرحلة يجب أن تشعر بأنها أسرع من السابقة.
             * نحافظ على جدول المرحلة الأساسي، ثم نضغط زمن الرحلة
             * تدريجياً فوقه بنسبة طفيفة وآمنة.
             */
            const float stageSpeedFactor =
                std::clamp(1.0f - static_cast<float>(mStageState.currentStageIndex) * 0.045f, 0.58f, 1.0f);
            spawnCtx.flightDurationMin = config.duckFlightDurationMin * stageSpeedFactor;
            spawnCtx.flightDurationMax = config.duckFlightDurationMax * stageSpeedFactor;
            spawnCtx.arcMinHeight = config.duckArcMinHeight;
            spawnCtx.arcMaxHeight = config.duckArcMaxHeight;
            spawnCtx.hunterX = mHunter.x;
            spawnCtx.metrics = metrics;
            mWaveManager.update(deltaTime, mDuckPool, spawnCtx);

            // صوت البطة
            if (mDuckPool.hasAnyFlying())
            {
                if (!mDuckAmbientSound.isPlaying())
                {
                    mDuckAmbientSound.playLooped();
                }
            }
            else
            {
                mDuckAmbientSound.stop();
            }

            // نجاح المرحلة الآن مرتبط بإكمال جميع بط المرحلة فعلياً،
            // حتى تتطابق الحسابات مع عداد البط المعروض في الواجهة.
            if (mStageState.ducksHitThisStage >= config.totalDucks)
            {
                endStage(true);
                break;
            }

            // فحص الفشل: نفدت الطلقات ولا يوجد بط نشط
            if (mStageState.shotsRemaining <= 0 && !mDuckPool.hasAnyActive())
            {
                endStage(false);
                break;
            }

            // فحص الفشل: كل البط ظهر واختفى ولم نصل للعدد المطلوب
            if (mWaveManager.isStageComplete())
            {
                endStage(false);
                break;
            }

            // فحص حد زمني
            if (config.timeLimitSeconds > 0.0f && mStageState.stageElapsedTime >= config.timeLimitSeconds)
            {
                endStage(false);
                break;
            }
            break;
        }

        case GamePhase::StageComplete:
            updateHunterMotion(deltaTime);
            mDuckPool.update(deltaTime);
            if (!mDuckPool.hasAnyActive() && mHunter.phase == hc::Phase::StageEnd)
            {
                mPhaseTimer -= deltaTime;
                if (mPhaseTimer <= 0.0f)
                {
                    mGamePhase = GamePhase::ResultsScreen;
                }
            }
            break;

        case GamePhase::StageFailed:
            updateHunterMotion(deltaTime);
            mDuckPool.update(deltaTime);
            if (!mDuckPool.hasAnyActive() && mHunter.phase == hc::Phase::StageEnd)
            {
                mGamePhase = GamePhase::ResultsScreen;
            }
            break;

        case GamePhase::ResultsScreen:
            if (mInput.isKeyJustPressed(GLFW_KEY_ENTER) || mInput.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT))
            {
                if (mLastStageResult.passed)
                {
                    int nextIndex = mStageState.currentStageIndex + 1;
                    if (nextIndex >= stage::kStageCount)
                    {
                        mGamePhase = GamePhase::GameVictory;
                    }
                    else
                    {
                        mGamePhase = GamePhase::StageIntro;
                        startStage(nextIndex);
                    }
                }
                else
                {
                    mGamePhase = GamePhase::StageIntro;
                    startStage(mStageState.currentStageIndex);
                }
            }
            break;

        case GamePhase::GameVictory:
            if (mInput.isKeyJustPressed(GLFW_KEY_ENTER) || mInput.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT))
            {
                mScore = 0;
                mRewardSystem.reset();
                mDifficultyMultiplier = 1.0f;
                mGamePhase = GamePhase::StageIntro;
                startStage(0);
            }
            break;
        }

        // تحديث بيانات العرض دائماً
        updateSoilRenderData();
        updateNatureRenderData();
        updateGrassRenderData();
        updateHunterRenderData();
        updateDuckRenderData();
        updateScoreRenderData();
        updateStageLabelRenderData();
        updateShotsDisplayRenderData();
        updateDucksRemainingRenderData();
        updateResultsRenderData();
        updateGroundInteraction(deltaTime);
    }

    void App::updateNatureSystem(float deltaTime)
    {
        mNatureSystem.update(deltaTime, scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight()));
    }

    void App::enterHunterStageEndPose()
    {
        /*
         * هذه هي النقطة الوحيدة المسموح بها لتثبيت الصياد
         * على وضعية ما بعد المرحلة. بذلك تصبح النهاية جزءاً
         * صريحاً من state machine بدلاً من flags جانبية.
         */
        hc::enterStageEndPose(mHunter, mSpriteAnim, mHunterState);
    }

    void App::updateHunterMotion(float deltaTime)
    {
        const game::HunterActionTiming &actionTiming = game::hunterActionTiming();
        hunterplay::updateReloadState(mHunter.isReloading, mHunter.reloadTimer, deltaTime);
        mHunter.shotCooldown = std::max(0.0f, mHunter.shotCooldown - deltaTime);
        hc::updateReadyPosture(mHunter, deltaTime);

        const bool stageEnding = mGamePhase == GamePhase::StageComplete || mGamePhase == GamePhase::StageFailed;
        const bool gameplayPhaseActive = (mGamePhase == GamePhase::Playing || mGamePhase == GamePhase::StageIntro);
        const bool gameplayInputAllowed = hc::isGameplayInputAllowed(gameplayPhaseActive, mHunter.phase);
        const int moveIntent = gameplayInputAllowed ? hunterplay::resolveMoveIntent(mInput) : 0;
        const bool reloadRequested = gameplayInputAllowed && mHunter.phase == hc::Phase::Locomotion &&
                                     mInput.isKeyJustPressed(GLFW_KEY_R);
        const bool canRequestShot =
            mGamePhase == GamePhase::Playing && mHunter.phase == hc::Phase::Locomotion &&
            !mHunter.isReloading && mHunter.shotCooldown <= 0.0f;
        const bool shotRequested = canRequestShot && mStageState.shotsRemaining > 0 &&
                                   mInput.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);

        const hunterplay::CursorScreenPosition cursor =
            hunterplay::resolveCursorScreenPosition(mInput, mWindow.getHandle());
        const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
        const game::AtlasFrame *baseFrame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
        const bool highShotRequested =
            shotRequested && hunterplay::shouldUseHighShootPose(mInput, mWindow.getHandle(), mHunter.x, metrics, baseFrame);

        const auto startShot = [&](bool highShot)
        {
            hc::resetReadyPosture(mHunter);
            mHunter.facingLeft = hunterplay::resolveFacingLeftFromCursor(cursor, mHunter.x, mHunter.facingLeft);
            mHunter.phase = highShot ? hc::Phase::ShootHigh : hc::Phase::Shoot;

            if (highShot)
            {
                hc::playDirectionalClip(mSpriteAnim, mHunterState, mHunter.facingLeft, "shoot_up_left", "shoot_up_right");
            }
            else
            {
                hc::playDirectionalClip(mSpriteAnim, mHunterState, mHunter.facingLeft, "shoot_left", "shoot_right");
            }

            --mStageState.shotsRemaining;
            mHunterShotSound.play();

            bool hitAny = false;
            if (cursor.valid)
            {
                DuckId hitId = mDuckPool.tryHit(cursor.x, cursor.y, metrics);
                if (hitId >= 0)
                {
                    hitAny = true;
                }
            }
            mSkillAssessment.onShotFired(hitAny);
        };

        const auto applyLocomotionClip = [&]() -> bool
        {
            if (gameplayInputAllowed)
            {
                mHunter.facingLeft = hunterplay::resolveIdleFacingLeft(moveIntent, cursor, mHunter.x, mHunter.facingLeft);
            }

            /*
             * الترقب للأعلى ليس حالة تجميد، بل posture فوق locomotion.
             * لذلك يمكن للصياد التحرك أفقياً مع إبقاء السلاح للأعلى
             * خلال المهلة المحددة، بدون الدخول في walk العادي.
             */
            if (mHunter.readyPosture != hc::ReadyPosture::Neutral && mHunter.readyPostureTimer > 0.0f)
            {
                if (moveIntent != 0)
                {
                    mHunter.x += static_cast<float>(moveIntent) * mHunter.speed * deltaTime;
                }
                hc::playReadyPostureClip(mSpriteAnim, mHunterState, mHunter);
                return false;
            }

            if (moveIntent != 0)
            {
                mHunter.x += static_cast<float>(moveIntent) * mHunter.speed * deltaTime;
                hc::playDirectionalClip(mSpriteAnim, mHunterState, mHunter.facingLeft, "walk_left", "walk_right");
                return true;
            }

            hc::playDirectionalClip(mSpriteAnim, mHunterState, mHunter.facingLeft, "idle_left", "idle_right");
            return false;
        };

        bool applyLocomotionState = false;
        bool preserveShotFacingOnLocomotionEntry = false;
        bool shouldAdvanceLocomotionAnimation = false;

        switch (mHunter.phase)
        {
        case hc::Phase::Locomotion:
            if (stageEnding)
            {
                enterHunterStageEndPose();
                break;
            }
            applyLocomotionState = !shotRequested;
            if (shotRequested)
            {
                startShot(highShotRequested);
            }
            else if (reloadRequested)
            {
                mHunter.isReloading = true;
                mHunter.reloadTimer = actionTiming.reloadDurationSeconds;
            }
            break;

        case hc::Phase::Shoot:
        case hc::Phase::ShootHigh:
            if (!mHunterState.finished)
            {
                mSpriteAnim.update(mHunterState, deltaTime);
                if (mHunterState.finished)
                {
                    const bool finishedHighShot = (mHunter.phase == hc::Phase::ShootHigh);
                    if (stageEnding)
                    {
                        enterHunterStageEndPose();
                    }
                    else
                    {
                        mHunter.phase = hc::Phase::Locomotion;
                        applyLocomotionState = true;
                        preserveShotFacingOnLocomotionEntry = true;
                        hc::setReadyPosture(mHunter,
                                            finishedHighShot ? hc::ReadyPosture::AimUp : hc::ReadyPosture::AimForward,
                                            actionTiming.readyPostureHoldDurationSeconds);
                    }
                }
            }
            else
            {
                const bool finishedHighShot = (mHunter.phase == hc::Phase::ShootHigh);
                if (stageEnding)
                {
                    enterHunterStageEndPose();
                }
                else
                {
                    mHunter.phase = hc::Phase::Locomotion;
                    applyLocomotionState = true;
                    preserveShotFacingOnLocomotionEntry = true;
                    hc::setReadyPosture(mHunter,
                                        finishedHighShot ? hc::ReadyPosture::AimUp : hc::ReadyPosture::AimForward,
                                        actionTiming.readyPostureHoldDurationSeconds);
                }
            }
            break;

        case hc::Phase::StageEnd:
            /*
             * نبقي الكليب مثبتاً على pose النهاية فقط.
             * لا توجد أي تحديثات زمنية أو إدخالات في هذه الحالة.
             */
            if (mHunterState.currentClip != (mHunter.facingLeft ? "stage_end_left" : "stage_end_right"))
            {
                hc::playDirectionalClip(mSpriteAnim, mHunterState, mHunter.facingLeft,
                                        "stage_end_left", "stage_end_right");
            }
            break;
        }

        if (applyLocomotionState && mHunter.phase == hc::Phase::Locomotion)
        {
            if (preserveShotFacingOnLocomotionEntry && moveIntent == 0)
            {
                hc::playReadyPostureClip(mSpriteAnim, mHunterState, mHunter);
            }
            else
            {
                shouldAdvanceLocomotionAnimation = applyLocomotionClip();
            }
            if (shouldAdvanceLocomotionAnimation)
            {
                mSpriteAnim.update(mHunterState, deltaTime);
            }
        }
    }

    void App::startStage(int stageIndex)
    {
        mStageState = {};
        mStageState.currentStageIndex = std::clamp(stageIndex, 0, stage::kStageCount - 1);
        const auto &config = stage::kStageTable[mStageState.currentStageIndex];
        mStageState.shotsRemaining = config.shotsAllowed;

        // طلقات إضافية من المكافآت
        const int bonusShots = mRewardSystem.consumeBonusShots();
        mStageState.shotsRemaining += bonusShots;
        mStageState.initialShots = mStageState.shotsRemaining;

        mPhaseTimer = 1.5f;

        // إعادة ضبط حالة الصياد ومنع إطلاق نار عرضي عند بداية المرحلة
        mHunter.phase = hc::Phase::Locomotion;
        mHunter.isReloading = false;
        mHunter.reloadTimer = 0.0f;
        mHunter.shotCooldown = 0.25f;
        hc::resetReadyPosture(mHunter);

        mDuckPool.clearAll();
        mSkillAssessment.reset();
        mWaveManager.startStage(config, mDifficultyMultiplier);
    }

    void App::endStage(bool won)
    {
        const auto &config = stage::kStageTable[mStageState.currentStageIndex];
        mStageState.stageActive = false;

        const auto& waveStats = mWaveManager.stats();
        const auto& extConfig = kExtendedStageTable[mStageState.currentStageIndex];

        mLastStageResult = {};
        mLastStageResult.stageNumber = config.stageNumber;
        mLastStageResult.passed = won;
        mLastStageResult.ducksHit = mStageState.ducksHitThisStage;
        mLastStageResult.ducksRequired = config.totalDucks;
        mLastStageResult.ducksTotal = config.totalDucks;
        mLastStageResult.ducksEscaped = waveStats.ducksDespawned - mStageState.ducksHitThisStage;
        mLastStageResult.shotsUsed = mStageState.initialShots - mStageState.shotsRemaining;
        mLastStageResult.shotsTotal = mStageState.initialShots;
        mLastStageResult.shotsRemaining = mStageState.shotsRemaining;
        mLastStageResult.timeTaken = mStageState.stageElapsedTime;
        mLastStageResult.shotsFired = mSkillAssessment.stats().shotsFired;
        mLastStageResult.accuracy = mSkillAssessment.accuracy();

        if (won)
        {
            mLastReward = mRewardSystem.computeReward(
                config, extConfig, mLastStageResult, mSkillAssessment.evaluate());
            mScore += mLastReward.bonusScore;
            mRewardSystem.onStageCompleted(mLastReward);
            mGamePhase = GamePhase::StageComplete;
            mPhaseTimer = 2.0f;
        }
        else
        {
            mLastReward = {};
            mRewardSystem.onStageFailed();
            mGamePhase = GamePhase::StageFailed;
        }

        /*
         * إذا لم يكن الصياد داخل كليب إطلاق عند لحظة نهاية المرحلة،
         * ندخل مباشرة وضعية النهاية. أما إذا كان يطلق النار فعلاً
         * فسيكمل كليب الإطلاق، ثم سيدخل StageEnd تلقائياً من state machine.
         */
        if (mHunter.phase == hc::Phase::Locomotion)
        {
            enterHunterStageEndPose();
        }

        // تحديث الصعوبة للمرحلة التالية
        mDifficultyMultiplier = mSkillAssessment.difficultyMultiplier();

        mDuckAmbientSound.stop();
    }

} // namespace core
