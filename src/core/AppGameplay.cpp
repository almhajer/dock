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
            spawnCtx.flightDurationMin = config.duckFlightDurationMin;
            spawnCtx.flightDurationMax = config.duckFlightDurationMax;
            spawnCtx.arcMinHeight = config.duckArcMinHeight;
            spawnCtx.arcMaxHeight = config.duckArcMaxHeight;
            spawnCtx.hunterX = mHunterX;
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
            mDuckPool.update(deltaTime);
            if (!mDuckPool.hasAnyActive())
            {
                mPhaseTimer -= deltaTime;
                if (mPhaseTimer <= 0.0f)
                {
                    mGamePhase = GamePhase::ResultsScreen;
                }
            }
            break;

        case GamePhase::StageFailed:
            mDuckPool.update(deltaTime);
            if (!mDuckPool.hasAnyActive())
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

    void App::updateHunterMotion(float deltaTime)
    {
        const game::HunterActionTiming &actionTiming = game::hunterActionTiming();
        hunterplay::updateReloadState(mIsReloading, mReloadTimer, deltaTime);
        mHunterShotRecoveryTimer = std::max(0.0f, mHunterShotRecoveryTimer - deltaTime);
        mShotCooldown = std::max(0.0f, mShotCooldown - deltaTime);

        const int moveIntent = hunterplay::resolveMoveIntent(mInput);
        const bool reloadRequested = mInput.isKeyJustPressed(GLFW_KEY_R);
        const bool shotRequested = mGamePhase == GamePhase::Playing && mStageState.shotsRemaining > 0 &&
                                   mInput.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) && !mIsReloading &&
                                   mShotCooldown <= 0.0f;

        if (moveIntent != 0)
        {
            mHunterX += static_cast<float>(moveIntent) * mHunterSpeed * deltaTime;
        }

        const hunterplay::CursorScreenPosition cursor =
            hunterplay::resolveCursorScreenPosition(mInput, mWindow.getHandle());
        const auto playDirectionalClip = [this](const char *leftClipKey, const char *rightClipKey, bool facingLeft)
        { mSpriteAnim.play(mHunterState, facingLeft ? leftClipKey : rightClipKey); };
        const auto enterIdlePosePreservingCurrentFrame = [&](bool facingLeft)
        {
            /*
             * عند التوقف بعد المشي نريد إبقاء آخر pose مرسوم كما هو،
             * لكن مع تحويل الحالة منطقياً إلى idle حتى لا يبقى النظام
             * يتعامل معه كمشي فعلي. هذا يمنع القفزة الرأسية البسيطة
             * التي تظهر عند الرجوع القسري إلى فريم idle الافتراضي.
             */
            const int preservedFrameIndex = mHunterState.currentFrameIndex;
            playDirectionalClip("idle_left", "idle_right", facingLeft);
            if (preservedFrameIndex >= 0)
            {
                mHunterState.currentFrameIndex = preservedFrameIndex;
            }
            mHunterState.elapsed = 0.0f;
            mHunterState.finished = false;
        };

        const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
        const game::AtlasFrame *baseFrame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
        const bool highShotRequested =
            shotRequested && hunterplay::shouldUseHighShootPose(mInput, mWindow.getHandle(), mHunterX, metrics, baseFrame);

        if (reloadRequested)
        {
            mIsReloading = true;
            mReloadTimer = actionTiming.reloadDurationSeconds;
        }

        const auto startShot = [&](bool highShot)
        {
            mFacingLeft = hunterplay::resolveFacingLeftFromCursor(cursor, mHunterX, mFacingLeft);
            mHunterPhase = highShot ? HunterPhase::ShootHigh : HunterPhase::Shoot;
            mShotCooldown = actionTiming.shotRecoveryDurationSeconds;

            if (highShot)
            {
                playDirectionalClip("shoot_up_left", "shoot_up_right", mFacingLeft);
            }
            else
            {
                playDirectionalClip("shoot_left", "shoot_right", mFacingLeft);
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
            const bool wasWalking = hunterplay::isWalkingClip(mHunterState);
            mFacingLeft = hunterplay::resolveIdleFacingLeft(moveIntent, cursor, mHunterX, mFacingLeft);

            if (moveIntent != 0)
            {
                playDirectionalClip("walk_left", "walk_right", mFacingLeft);
                return true;
            }

            if (wasWalking)
            {
                enterIdlePosePreservingCurrentFrame(mFacingLeft);
                return false;
            }

            playDirectionalClip("idle_left", "idle_right", mFacingLeft);
            return false;
        };

        bool applyLocomotionState = false;
        bool preserveShotFacingOnLocomotionEntry = false;
        bool shouldAdvanceLocomotionAnimation = false;

        switch (mHunterPhase)
        {
        case HunterPhase::Locomotion:
            applyLocomotionState = !shotRequested;
            if (shotRequested)
            {
                startShot(highShotRequested);
            }
            break;

        case HunterPhase::Shoot:
        case HunterPhase::ShootHigh:
            if (!mHunterState.finished)
            {
                mSpriteAnim.update(mHunterState, deltaTime);
                if (mHunterState.finished)
                {
                    mHunterShotRecoveryTimer = actionTiming.shotRecoveryDurationSeconds;
                    mHunterPhase = HunterPhase::Locomotion;
                    applyLocomotionState = true;
                    preserveShotFacingOnLocomotionEntry = true;
                }
            }
            else if (shotRequested)
            {
                mHunterShotRecoveryTimer = 0.0f;
                startShot(highShotRequested);
            }
            else
            {
                mHunterPhase = HunterPhase::Locomotion;
                applyLocomotionState = true;
                preserveShotFacingOnLocomotionEntry = true;
            }
            break;
        }

        if (applyLocomotionState && mHunterPhase == HunterPhase::Locomotion)
        {
            if (preserveShotFacingOnLocomotionEntry && moveIntent == 0)
            {
                playDirectionalClip("idle_left", "idle_right", mFacingLeft);
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
        mHunterPhase = HunterPhase::Locomotion;
        mIsReloading = false;
        mReloadTimer = 0.0f;
        mHunterShotRecoveryTimer = 0.0f;
        mShotCooldown = 0.25f;

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

        // تحديث الصعوبة للمرحلة التالية
        mDifficultyMultiplier = mSkillAssessment.difficultyMultiplier();

        mDuckAmbientSound.stop();
    }

} // namespace core
