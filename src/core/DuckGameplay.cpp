#include "DuckGameplay.h"

#include <algorithm>
#include <cmath>

namespace core::duckplay {

float smoothstep01(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float remapTravelPhase(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    if (t <= kDuckLaunchPhaseRatio)
    {
        const float localT = t / kDuckLaunchPhaseRatio;
        return smoothstep01(localT) * kDuckLaunchDistanceRatio;
    }

    const float localT =
        (t - kDuckLaunchPhaseRatio) / (1.0f - kDuckLaunchPhaseRatio);
    return kDuckLaunchDistanceRatio +
        smoothstep01(localT) * (1.0f - kDuckLaunchDistanceRatio);
}

float cubicBezier1D(float p0, float p1, float p2, float p3, float t)
{
    const float oneMinusT = 1.0f - t;
    return oneMinusT * oneMinusT * oneMinusT * p0 +
           3.0f * oneMinusT * oneMinusT * t * p1 +
           3.0f * oneMinusT * t * t * p2 +
           t * t * t * p3;
}

float cubicBezierTangent1D(float p0, float p1, float p2, float p3, float t)
{
    const float oneMinusT = 1.0f - t;
    return 3.0f * oneMinusT * oneMinusT * (p1 - p0) +
           6.0f * oneMinusT * t * (p2 - p1) +
           3.0f * t * t * (p3 - p2);
}

void holdDuckOnGroundHitFrame(game::AnimationState& state)
{
    const game::AnimationClip* clip = state.activeClip;
    if (clip == nullptr || clip->frames.empty())
    {
        return;
    }

    const std::size_t groundedFrameIndex =
        (clip->frames.size() >= 4u) ? 3u : (clip->frames.size() - 1u);
    state.currentFrameIndex = clip->frames[groundedFrameIndex];
    state.finished = true;
}

} // namespace core::duckplay
