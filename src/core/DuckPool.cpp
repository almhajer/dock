#include "DuckPool.h"
#include "SceneLayout.h"

#include <algorithm>
#include <cmath>

namespace core
{

void DuckPool::initialize(game::SpriteAnimation* anim, std::mt19937* rng)
{
    mAnim = anim;
    mRng = rng;
    clearAll();
}

DuckId DuckPool::spawn(const DuckSpawnContext& ctx)
{
    for (int i = 0; i < kMaxSimultaneousDucks; ++i)
    {
        if (!mDucks[i].isActive())
        {
            randomizeFlight(mDucks[i], ctx);
            return i;
        }
    }
    return -1;
}

void DuckPool::release(DuckId id)
{
    if (id >= 0 && id < kMaxSimultaneousDucks)
    {
        mDucks[id].phase = DuckPhase::Inactive;
    }
}

void DuckPool::update(float deltaTime)
{
    for (int i = 0; i < kMaxSimultaneousDucks; ++i)
    {
        DuckInstance& duck = mDucks[i];
        if (!duck.isActive())
        {
            continue;
        }

        switch (duck.phase)
        {
        case DuckPhase::Flying:
            updateFlyingDuck(duck, deltaTime);
            break;
        case DuckPhase::Falling:
            updateFallingDuck(duck, deltaTime);
            break;
        case DuckPhase::Grounded:
            updateGroundedDuck(duck, deltaTime);
            break;
        case DuckPhase::Inactive:
            break;
        }
    }
}

DuckId DuckPool::tryHit(float cursorX, float cursorY, const scene::WindowMetrics& metrics)
{
    for (int i = 0; i < kMaxSimultaneousDucks; ++i)
    {
        DuckInstance& duck = mDucks[i];
        if (duck.phase != DuckPhase::Flying || duck.animState.currentClip.empty())
        {
            continue;
        }

        const game::AtlasFrame* frame = mAnim->getFrame(duck.animState.currentFrameIndex);
        if (frame == nullptr || frame->sourceW <= 0 || frame->sourceH <= 0)
        {
            continue;
        }

        const gfx::TexturedQuad duckQuad =
            scene::buildSpriteQuad(*frame, duck.x, duck.y, duckplay::kDuckScreenHalfHeight, metrics);
        if (cursorX >= duckQuad.screen.x0 && cursorX <= duckQuad.screen.x1 &&
            cursorY >= duckQuad.screen.y0 && cursorY <= duckQuad.screen.y1)
        {
            const bool movingRight = duck.flight.endX >= duck.flight.startX;
            duck.phase = DuckPhase::Falling;
            duck.facingLeft = !movingRight;
            duck.velocityX = movingRight ? duckplay::kDuckFallHorizontalDrift : -duckplay::kDuckFallHorizontalDrift;
            duck.velocityY = duckplay::kDuckFallInitialLift;
            duck.angularVelocity = movingRight ? duckplay::kDuckFallAngularSpeed : -duckplay::kDuckFallAngularSpeed;
            duck.groundTimer = 0.0f;
            duck.flipY = false;
            duck.alpha = 1.0f;
            mAnim->play(duck.animState, duck.facingLeft ? "hit_left" : "hit_right");
            ++mTotalHit;
            return i;
        }
    }
    return -1;
}

void DuckPool::clearAll()
{
    for (auto& duck : mDucks)
    {
        duck.phase = DuckPhase::Inactive;
        duck.animState.currentClip.clear();
    }
    mTotalHit = 0;
    mTotalDespawned = 0;
}

void DuckPool::buildRenderQuads(std::vector<gfx::TexturedQuad>& quads,
                                const scene::WindowMetrics& metrics) const
{
    for (int i = 0; i < kMaxSimultaneousDucks; ++i)
    {
        const DuckInstance& duck = mDucks[i];
        if (!duck.isActive() || duck.animState.currentClip.empty())
        {
            continue;
        }

        const game::AtlasFrame* frame = mAnim->getFrame(duck.animState.currentFrameIndex);
        if (frame == nullptr || frame->sourceW <= 0 || frame->sourceH <= 0)
        {
            continue;
        }

        gfx::UvRect uv;
        mAnim->getFrameUV(duck.animState.currentFrameIndex, uv.u0, uv.u1, uv.v0, uv.v1);
        if (duck.facingLeft)
        {
            std::swap(uv.u0, uv.u1);
        }
        if (duck.flipY)
        {
            std::swap(uv.v0, uv.v1);
        }

        gfx::TexturedQuad quad = scene::buildSpriteQuad(*frame, duck.x, duck.y,
                                                         duckplay::kDuckScreenHalfHeight, metrics);
        quad.uv = uv;
        quad.alpha = duck.alpha;
        quad.rotationRadians = duck.rotation;
        quads.push_back(quad);
    }
}

int DuckPool::activeCount() const
{
    int count = 0;
    for (const auto& duck : mDucks)
    {
        if (duck.isActive())
        {
            ++count;
        }
    }
    return count;
}

bool DuckPool::hasAnyFlying() const
{
    for (const auto& duck : mDucks)
    {
        if (duck.phase == DuckPhase::Flying)
        {
            return true;
        }
    }
    return false;
}

bool DuckPool::hasAnyActive() const
{
    for (const auto& duck : mDucks)
    {
        if (duck.isActive())
        {
            return true;
        }
    }
    return false;
}

void DuckPool::updateFlyingDuck(DuckInstance& duck, float deltaTime)
{
    duck.flightTime += deltaTime;
    if (duck.flight.duration > 0.0f && duck.flightTime >= duck.flight.duration)
    {
        duck.phase = DuckPhase::Inactive;
        ++mTotalDespawned;
        return;
    }

    const float cycleT = std::clamp(duck.flightTime / std::max(duck.flight.duration, 0.001f), 0.0f, 1.0f);
    const float motionT = duckplay::remapTravelPhase(cycleT);
    const bool movingRight = duck.flight.endX >= duck.flight.startX;
    const float pathY = duckplay::cubicBezier1D(duck.flight.startY, duck.flight.controlY1,
                                                  duck.flight.controlY2, duck.flight.endY, motionT);
    const float tangentY = duckplay::cubicBezierTangent1D(duck.flight.startY, duck.flight.controlY1,
                                                            duck.flight.controlY2, duck.flight.endY, motionT);
    const float normalizedTravel = std::max(std::abs(duck.flight.endX - duck.flight.startX), 0.001f);
    const float climbBias = std::clamp(-tangentY / normalizedTravel * 5.2f, -1.0f, 1.0f);
    const float glideBlend = std::sin(motionT * duckplay::kPi);
    const float flapEffort = 0.35f + (1.0f - glideBlend) * 0.65f;
    const float flapPhase = motionT * duckplay::kTau * duck.flight.flapCycles;
    const float flapBob = std::sin(flapPhase) * duckplay::kDuckFlapBobAmplitude * flapEffort;
    const float sway = std::sin(motionT * duckplay::kTau + duck.flight.swayPhase) *
                       duckplay::kDuckSwayAmplitude * (0.4f + glideBlend * 0.6f);

    duck.x = std::lerp(duck.flight.startX, duck.flight.endX, motionT);
    duck.y = pathY + sway + flapBob - glideBlend * duckplay::kDuckGlideLift;

    const float facingSign = movingRight ? 1.0f : -1.0f;
    const float pathRotation = std::clamp(climbBias * duckplay::kDuckFlightRotationMax,
                                           -duckplay::kDuckFlightRotationMax, duckplay::kDuckFlightRotationMax);
    const float flapRoll = std::sin(flapPhase + 0.45f) * duckplay::kDuckFlapRollMax * flapEffort;
    duck.rotation = facingSign * (pathRotation + flapRoll);
    duck.flipY = false;
    duck.alpha = 1.0f;

    const bool facingLeft = !movingRight;
    const char* clipKey = facingLeft ? "fly_left" : "fly_right";
    if (duck.facingLeft != facingLeft || duck.animState.currentClip != clipKey)
    {
        duck.facingLeft = facingLeft;
        mAnim->play(duck.animState, clipKey);
    }

    const float flapPlaybackScale = 0.58f + flapEffort * 0.42f;
    mAnim->update(duck.animState, deltaTime * flapPlaybackScale);
}

void DuckPool::updateFallingDuck(DuckInstance& duck, float deltaTime)
{
    duck.velocityY += duckplay::kDuckFallGravity * deltaTime;
    duck.x += duck.velocityX * deltaTime;
    duck.y += duck.velocityY * deltaTime;
    duck.rotation += duck.angularVelocity * deltaTime;
    duck.flipY = std::cos(duck.rotation * 0.75f) < 0.0f;
    mAnim->update(duck.animState, deltaTime);

    const float groundPivotY = scene::groundSurfaceY() - duckplay::kDuckScreenHalfHeight;
    if (duck.y >= groundPivotY)
    {
        duck.y = groundPivotY;
        duck.velocityX = 0.0f;
        duck.velocityY = 0.0f;
        duck.angularVelocity = 0.0f;
        duck.groundTimer = 0.0f;
        duck.phase = DuckPhase::Grounded;
        duck.rotation = 0.0f;
        duck.flipY = false;
        duck.alpha = 1.0f;
        duckplay::holdDuckOnGroundHitFrame(duck.animState);
    }
}

void DuckPool::updateGroundedDuck(DuckInstance& duck, float deltaTime)
{
    duck.groundTimer += deltaTime;
    duck.rotation = 0.0f;
    duck.flipY = false;
    duck.alpha = 1.0f - duckplay::smoothstep01(
                            std::clamp(duck.groundTimer / duckplay::kDuckGroundHideDelaySeconds, 0.0f, 1.0f));
    duckplay::holdDuckOnGroundHitFrame(duck.animState);
    if (duck.groundTimer >= duckplay::kDuckGroundHideDelaySeconds)
    {
        duck.phase = DuckPhase::Inactive;
        ++mTotalDespawned;
    }
}

void DuckPool::randomizeFlight(DuckInstance& duck, const DuckSpawnContext& ctx)
{
    const float topLimit = -1.0f + duckplay::kDuckTopSpawnMargin;
    float bottomLimit = -0.28f;
    if (ctx.metrics.valid())
    {
        bottomLimit = std::clamp(bottomLimit, topLimit + 0.06f, 0.05f);
    }

    const float yRange = bottomLimit - topLimit;
    const int waveSize = std::max(ctx.waveSize, 1);
    const int idx = std::clamp(ctx.duckIndex, 0, waveSize - 1);

    // توزيع ارتفاعات الدفعة على شرائح متساوية مع عشوائية داخل كل شريحة
    const float segmentH = yRange / static_cast<float>(waveSize);
    const float segTop = topLimit + static_cast<float>(idx) * segmentH;
    const float segBottom = segTop + segmentH;
    std::uniform_real_distribution<float> yDist(segTop, segBottom);

    std::uniform_real_distribution<float> arcDist(ctx.arcMinHeight, ctx.arcMaxHeight);
    std::uniform_real_distribution<float> durDist(ctx.flightDurationMin, ctx.flightDurationMax);
    std::uniform_real_distribution<float> ctrl1Dist(duckplay::kDuckControl1MinRatio,
                                                     duckplay::kDuckControl1MaxRatio);
    std::uniform_real_distribution<float> ctrl2Dist(duckplay::kDuckControl2MinRatio,
                                                     duckplay::kDuckControl2MaxRatio);
    std::uniform_real_distribution<float> flapDist(duckplay::kDuckFlapCyclesMin, duckplay::kDuckFlapCyclesMax);
    std::uniform_real_distribution<float> phaseDist(0.0f, duckplay::kTau);

    // تبديل الجهات: بطة زوجية من اليسار، فردية من اليمين
    const bool startFromLeft = (idx % 2 == 0);
    duck.flight.startX = startFromLeft ? (-1.0f - duckplay::kDuckTravelMargin)
                                        : (1.0f + duckplay::kDuckTravelMargin);
    duck.flight.endX = -duck.flight.startX;
    duck.flight.startY = yDist(*mRng);
    duck.flight.endY = yDist(*mRng);
    duck.flight.arcHeight = arcDist(*mRng);
    duck.flight.duration = durDist(*mRng);

    const float highestY = std::min(duck.flight.startY, duck.flight.endY) - duck.flight.arcHeight;
    const float ctrl1Ratio = ctrl1Dist(*mRng);
    const float ctrl2Ratio = ctrl2Dist(*mRng);
    duck.flight.controlY1 = std::lerp(duck.flight.startY, highestY, ctrl1Ratio);
    duck.flight.controlY2 = std::lerp(duck.flight.endY, highestY, ctrl2Ratio);
    duck.flight.flapCycles = flapDist(*mRng);
    duck.flight.swayPhase = phaseDist(*mRng);

    duck.phase = DuckPhase::Flying;
    duck.flightTime = 0.0f;
    duck.x = duck.flight.startX;
    duck.y = duck.flight.startY;
    duck.velocityX = 0.0f;
    duck.velocityY = 0.0f;
    duck.rotation = 0.0f;
    duck.angularVelocity = 0.0f;
    duck.groundTimer = 0.0f;
    duck.flipY = false;
    duck.alpha = 1.0f;
    duck.facingLeft = duck.flight.endX < duck.flight.startX;
    mAnim->play(duck.animState, duck.facingLeft ? "fly_left" : "fly_right");
}

} // namespace core
