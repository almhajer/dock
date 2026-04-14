#include "HunterController.h"

#include <algorithm>

namespace core::huntercontroller
{

bool isGameplayInputAllowed(bool gameplayPhaseActive, Phase phase)
{
    return gameplayPhaseActive && phase != Phase::StageEnd;
}

void updateReadyPosture(State& state, float deltaTime)
{
    state.readyPostureTimer = std::max(0.0f, state.readyPostureTimer - deltaTime);
    if (state.readyPostureTimer <= 0.0f)
    {
        state.readyPosture = ReadyPosture::Neutral;
    }
}

void resetReadyPosture(State& state)
{
    state.readyPosture = ReadyPosture::Neutral;
    state.readyPostureTimer = 0.0f;
}

void setReadyPosture(State& state, ReadyPosture posture, float durationSeconds)
{
    state.readyPosture = posture;
    state.readyPostureTimer = std::max(0.0f, durationSeconds);
}

void playDirectionalClip(game::SpriteAnimation& animation, game::AnimationState& animState,
                         bool facingLeft, const char* leftClipKey, const char* rightClipKey)
{
    animation.play(animState, facingLeft ? leftClipKey : rightClipKey);
}

void playReadyPostureClip(game::SpriteAnimation& animation, game::AnimationState& animState, const State& state)
{
    switch (state.readyPosture)
    {
    case ReadyPosture::AimForward:
        playDirectionalClip(animation, animState, state.facingLeft, "ready_forward_left", "ready_forward_right");
        break;
    case ReadyPosture::AimUp:
        playDirectionalClip(animation, animState, state.facingLeft, "ready_up_left", "ready_up_right");
        break;
    case ReadyPosture::Neutral:
    default:
        playDirectionalClip(animation, animState, state.facingLeft, "idle_left", "idle_right");
        break;
    }
}

void enterStageEndPose(State& state, game::SpriteAnimation& animation, game::AnimationState& animState)
{
    state.phase = Phase::StageEnd;
    state.isReloading = false;
    state.reloadTimer = 0.0f;
    state.shotCooldown = 0.0f;
    resetReadyPosture(state);
    playDirectionalClip(animation, animState, state.facingLeft, "stage_end_left", "stage_end_right");
}

} // namespace core::huntercontroller
