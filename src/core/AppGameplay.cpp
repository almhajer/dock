#include "App.h"
#include "DuckGameplay.h"
#include "HunterGameplay.h"
#include "SceneLayout.h"
#include "../game/HunterSpriteData.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

namespace core {

/*
 * هذا الملف مسؤول عن منطق اللعب:
 * - تحديث حالة الصياد
 * - تحديث مسار البطة
 * - كشف الإصابة والتحويل بين الحالات
 */

void App::update(float deltaTime)
{
    updateNatureSystem(deltaTime);
    updateDuckMotion(deltaTime);
    updateHunterMotion(deltaTime);
    updateSoilRenderData();
    updateNatureRenderData();
    updateGrassRenderData();
    updateHunterRenderData();
    updateDuckRenderData();
    updateGroundInteraction(deltaTime);
}

void App::updateNatureSystem(float deltaTime)
{
    mNatureSystem.update(
        deltaTime,
        scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight()));
}

void App::updateHunterMotion(float deltaTime)
{
    const game::HunterActionTiming& actionTiming = game::hunterActionTiming();
    hunterplay::updateReloadState(mIsReloading, mReloadTimer, deltaTime);
    mHunterShotRecoveryTimer = std::max(0.0f, mHunterShotRecoveryTimer - deltaTime);

    const bool shotRecoveryActive = mHunterShotRecoveryTimer > 0.0f;
    const int moveIntent = hunterplay::resolveMoveIntent(mInput);
    const bool reloadRequested = mInput.isKeyJustPressed(GLFW_KEY_R);
    const bool shotRequested =
        mInput.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) &&
        !mIsReloading;

    if (moveIntent != 0)
    {
        mHunterX += static_cast<float>(moveIntent) * mHunterSpeed * deltaTime;
    }

    const hunterplay::CursorScreenPosition cursor =
        hunterplay::resolveCursorScreenPosition(mInput, mWindow.getHandle());
    const auto playDirectionalClip = [this](const char* leftClipKey,
                                            const char* rightClipKey,
                                            bool facingLeft)
    {
        mSpriteAnim.play(mHunterState, facingLeft ? leftClipKey : rightClipKey);
    };

    const scene::WindowMetrics metrics =
        scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    const game::AtlasFrame* baseFrame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
    const bool highShotRequested = shotRequested &&
        hunterplay::shouldUseHighShootPose(
            mInput, mWindow.getHandle(), mHunterX, metrics, baseFrame);

    if (reloadRequested)
    {
        mIsReloading = true;
        mReloadTimer = actionTiming.reloadDurationSeconds;
    }

    const auto startShot = [&](bool highShot)
    {
        mFacingLeft = hunterplay::resolveFacingLeftFromCursor(cursor, mHunterX, mFacingLeft);
        mHunterPhase = highShot ? HunterPhase::ShootHigh : HunterPhase::Shoot;

        if (highShot)
        {
            playDirectionalClip("shoot_up_left", "shoot_up_right", mFacingLeft);
        }
        else
        {
            playDirectionalClip("shoot_left", "shoot_right", mFacingLeft);
        }

        mHunterShotSound.play();
        if (cursor.valid)
        {
            static_cast<void>(tryHitDuck(cursor.x, cursor.y));
        }
    };

    const auto applyLocomotionClip = [&]()
    {
        mFacingLeft = hunterplay::resolveIdleFacingLeft(moveIntent, cursor, mHunterX, mFacingLeft);

        if (moveIntent != 0)
        {
            playDirectionalClip("walk_left", "walk_right", mFacingLeft);
        }
        else
        {
            playDirectionalClip("idle_left", "idle_right", mFacingLeft);
        }
    };

    bool applyLocomotionState = false;
    bool preserveShotFacingOnLocomotionEntry = false;

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
                hunterplay::holdHunterOnShotStartFrame(mHunterState);
            }
        }
        else if (shotRequested)
        {
            mHunterShotRecoveryTimer = 0.0f;
            startShot(highShotRequested);
        }
        else if (shotRecoveryActive)
        {
            hunterplay::holdHunterOnShotStartFrame(mHunterState);
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
            applyLocomotionClip();
        }
        mSpriteAnim.update(mHunterState, deltaTime);
    }
}

void App::updateDuckMotion(float deltaTime)
{
    if (mDuckState.currentClip.empty())
    {
        mDuckAmbientSound.stop();
        return;
    }

    if (mDuckPhase == DuckPhase::Flying)
    {
        mDuckFlightTime += deltaTime;
        while (mDuckFlightDuration > 0.0f && mDuckFlightTime >= mDuckFlightDuration)
        {
            mDuckFlightTime -= mDuckFlightDuration;
            resetDuckFlight();
        }

        const float cycleT = std::clamp(
            mDuckFlightTime / std::max(mDuckFlightDuration, 0.001f),
            0.0f,
            1.0f);
        const float motionT = duckplay::remapTravelPhase(cycleT);
        const bool movingRight = mDuckFlightEndX >= mDuckFlightStartX;
        const float pathY = duckplay::cubicBezier1D(
            mDuckFlightStartY,
            mDuckFlightControlY1,
            mDuckFlightControlY2,
            mDuckFlightEndY,
            motionT);
        const float tangentY = duckplay::cubicBezierTangent1D(
            mDuckFlightStartY,
            mDuckFlightControlY1,
            mDuckFlightControlY2,
            mDuckFlightEndY,
            motionT);
        const float normalizedTravel = std::max(
            std::abs(mDuckFlightEndX - mDuckFlightStartX),
            0.001f);
        const float climbBias =
            std::clamp(-tangentY / normalizedTravel * 5.2f, -1.0f, 1.0f);
        const float glideBlend = std::sin(motionT * duckplay::kPi);
        const float flapEffort = 0.35f + (1.0f - glideBlend) * 0.65f;
        const float flapPhase = motionT * duckplay::kTau * mDuckFlightFlapCycles;
        const float flapBob =
            std::sin(flapPhase) * duckplay::kDuckFlapBobAmplitude * flapEffort;
        const float sway =
            std::sin(motionT * duckplay::kTau + mDuckFlightSwayPhase) *
            duckplay::kDuckSwayAmplitude *
            (0.4f + glideBlend * 0.6f);

        mDuckX = std::lerp(mDuckFlightStartX, mDuckFlightEndX, motionT);
        mDuckY = pathY + sway + flapBob - glideBlend * duckplay::kDuckGlideLift;
        const float facingSign = movingRight ? 1.0f : -1.0f;
        const float pathRotation = std::clamp(
            climbBias * duckplay::kDuckFlightRotationMax,
            -duckplay::kDuckFlightRotationMax,
            duckplay::kDuckFlightRotationMax);
        const float flapRoll =
            std::sin(flapPhase + 0.45f) * duckplay::kDuckFlapRollMax * flapEffort;
        mDuckRotation = facingSign * (pathRotation + flapRoll);
        mDuckFlipY = false;
        mDuckAlpha = 1.0f;

        const bool facingLeft = !movingRight;
        const char* clipKey = facingLeft ? "fly_left" : "fly_right";
        if (mDuckFacingLeft != facingLeft || mDuckState.currentClip != clipKey)
        {
            mDuckFacingLeft = facingLeft;
            mDuckAnim.play(mDuckState, clipKey);
        }

        const float flapPlaybackScale = 0.58f + flapEffort * 0.42f;
        mDuckAnim.update(mDuckState, deltaTime * flapPlaybackScale);
        return;
    }

    if (mDuckPhase == DuckPhase::Falling)
    {
        mDuckVelocityY += duckplay::kDuckFallGravity * deltaTime;
        mDuckX += mDuckVelocityX * deltaTime;
        mDuckY += mDuckVelocityY * deltaTime;
        mDuckRotation += mDuckAngularVelocity * deltaTime;
        mDuckFlipY = std::cos(mDuckRotation * 0.75f) < 0.0f;
        mDuckAnim.update(mDuckState, deltaTime);

        const float groundPivotY = scene::groundSurfaceY() - duckplay::kDuckScreenHalfHeight;
        if (mDuckY >= groundPivotY)
        {
            mDuckY = groundPivotY;
            mDuckVelocityX = 0.0f;
            mDuckVelocityY = 0.0f;
            mDuckAngularVelocity = 0.0f;
            mDuckGroundTimer = 0.0f;
            mDuckPhase = DuckPhase::Grounded;
            mDuckRotation = 0.0f;
            mDuckFlipY = false;
            mDuckAlpha = 1.0f;
            duckplay::holdDuckOnGroundHitFrame(mDuckState);
        }
        return;
    }

    mDuckGroundTimer += deltaTime;
    mDuckRotation = 0.0f;
    mDuckFlipY = false;
    mDuckAlpha = 1.0f - duckplay::smoothstep01(std::clamp(
        mDuckGroundTimer / duckplay::kDuckGroundHideDelaySeconds,
        0.0f,
        1.0f));
    duckplay::holdDuckOnGroundHitFrame(mDuckState);
    if (mDuckGroundTimer >= duckplay::kDuckGroundHideDelaySeconds)
    {
        resetDuckFlight();
    }
}

void App::resetDuckFlight()
{
    const scene::WindowMetrics metrics =
        scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    const game::AtlasFrame* hunterFrame =
        mSpriteAnim.getFrame(mHunterState.currentFrameIndex);

    const float topLimit = -1.0f + duckplay::kDuckTopSpawnMargin;
    float bottomLimit = -0.28f;
    if (metrics.valid() &&
        hunterFrame != nullptr &&
        hunterFrame->sourceW > 0 &&
        hunterFrame->sourceH > 0)
    {
        const gfx::TexturedQuad hunterQuad =
            scene::buildHunterQuad(*hunterFrame, mHunterX, metrics);
        bottomLimit = hunterQuad.screen.y0 - duckplay::kDuckHunterClearance;
    }

    bottomLimit = std::clamp(bottomLimit, topLimit + 0.06f, 0.05f);

    std::uniform_real_distribution<float> yDistribution(topLimit, bottomLimit);
    std::uniform_real_distribution<float> arcDistribution(
        duckplay::kDuckArcMinHeight,
        duckplay::kDuckArcMaxHeight);
    std::uniform_real_distribution<float> durationDistribution(
        duckplay::kDuckFlightMinSeconds,
        duckplay::kDuckFlightMaxSeconds);
    std::uniform_real_distribution<float> control1RatioDistribution(
        duckplay::kDuckControl1MinRatio,
        duckplay::kDuckControl1MaxRatio);
    std::uniform_real_distribution<float> control2RatioDistribution(
        duckplay::kDuckControl2MinRatio,
        duckplay::kDuckControl2MaxRatio);
    std::uniform_real_distribution<float> flapCycleDistribution(
        duckplay::kDuckFlapCyclesMin,
        duckplay::kDuckFlapCyclesMax);
    std::uniform_real_distribution<float> phaseDistribution(0.0f, duckplay::kTau);
    std::bernoulli_distribution directionDistribution(0.5);

    const bool startFromLeft = directionDistribution(mDuckRandomEngine);
    mDuckFlightStartX = startFromLeft
        ? (-1.0f - duckplay::kDuckTravelMargin)
        : (1.0f + duckplay::kDuckTravelMargin);
    mDuckFlightEndX = -mDuckFlightStartX;
    mDuckFlightStartY = yDistribution(mDuckRandomEngine);
    mDuckFlightEndY = yDistribution(mDuckRandomEngine);
    mDuckFlightArcHeight = arcDistribution(mDuckRandomEngine);
    mDuckFlightDuration = durationDistribution(mDuckRandomEngine);
    const float highestY =
        std::min(mDuckFlightStartY, mDuckFlightEndY) - mDuckFlightArcHeight;
    const float control1Ratio = control1RatioDistribution(mDuckRandomEngine);
    const float control2Ratio = control2RatioDistribution(mDuckRandomEngine);
    mDuckFlightControlY1 = std::lerp(mDuckFlightStartY, highestY, control1Ratio);
    mDuckFlightControlY2 = std::lerp(mDuckFlightEndY, highestY, control2Ratio);
    mDuckFlightFlapCycles = flapCycleDistribution(mDuckRandomEngine);
    mDuckFlightSwayPhase = phaseDistribution(mDuckRandomEngine);

    mDuckPhase = DuckPhase::Flying;
    mDuckFlightTime = 0.0f;
    mDuckVelocityX = 0.0f;
    mDuckVelocityY = 0.0f;
    mDuckRotation = 0.0f;
    mDuckAngularVelocity = 0.0f;
    mDuckGroundTimer = 0.0f;
    mDuckFlipY = false;
    mDuckAlpha = 1.0f;
    mDuckX = mDuckFlightStartX;
    mDuckY = mDuckFlightStartY;
    mDuckFacingLeft = mDuckFlightEndX < mDuckFlightStartX;
    mDuckAnim.play(mDuckState, mDuckFacingLeft ? "fly_left" : "fly_right");
    if (!mDuckAmbientSound.isPlaying())
    {
        mDuckAmbientSound.playLooped();
    }
}

bool App::tryHitDuck(float cursorX, float cursorY)
{
    if (mDuckPhase != DuckPhase::Flying || mDuckState.currentClip.empty())
    {
        return false;
    }

    const scene::WindowMetrics metrics =
        scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        return false;
    }

    const game::AtlasFrame* frame = mDuckAnim.getFrame(mDuckState.currentFrameIndex);
    if (frame == nullptr || frame->sourceW <= 0 || frame->sourceH <= 0)
    {
        return false;
    }

    const gfx::TexturedQuad duckQuad = scene::buildSpriteQuad(
        *frame,
        mDuckX,
        mDuckY,
        duckplay::kDuckScreenHalfHeight,
        metrics);
    if (cursorX < duckQuad.screen.x0 || cursorX > duckQuad.screen.x1 ||
        cursorY < duckQuad.screen.y0 || cursorY > duckQuad.screen.y1)
    {
        return false;
    }

    const bool movingRight = mDuckFlightEndX >= mDuckFlightStartX;
    mDuckAmbientSound.stop();
    mDuckPhase = DuckPhase::Falling;
    mDuckFacingLeft = !movingRight;
    mDuckVelocityX = movingRight
        ? duckplay::kDuckFallHorizontalDrift
        : -duckplay::kDuckFallHorizontalDrift;
    mDuckVelocityY = duckplay::kDuckFallInitialLift;
    mDuckAngularVelocity = movingRight
        ? duckplay::kDuckFallAngularSpeed
        : -duckplay::kDuckFallAngularSpeed;
    mDuckGroundTimer = 0.0f;
    mDuckFlipY = false;
    mDuckAlpha = 1.0f;
    mDuckAnim.play(mDuckState, mDuckFacingLeft ? "hit_left" : "hit_right");
    return true;
}

} // namespace core
