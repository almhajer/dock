#pragma once

#include "../game/SpriteAnimation.h"

namespace core::huntercontroller
{

/*
 * مرحلة الفعل الأساسية للصياد.
 */
enum class Phase : unsigned char
{
    Locomotion,
    Shoot,
    ShootHigh,
    StageEnd,
};

/*
 * وضعية الترقب البصرية فوق الحركة.
 */
enum class ReadyPosture : unsigned char
{
    Neutral,
    AimForward,
    AimUp,
};

/*
 * الحالة التشغيلية الكاملة الخاصة بالصياد.
 * وجودها في struct واحد يسهل تمريرها وإعادة استخدامها
 * ويمنع تشتيت الحقول داخل App.
 */
struct State
{
    float x = 0.0f;
    float speed = 0.8f;
    float reloadTimer = 0.0f;
    float shotCooldown = 0.0f;
    bool facingLeft = false;
    bool isReloading = false;
    ReadyPosture readyPosture = ReadyPosture::Neutral;
    float readyPostureTimer = 0.0f;
    Phase phase = Phase::Locomotion;
};

[[nodiscard]] bool isGameplayInputAllowed(bool gameplayPhaseActive, Phase phase);
void updateReadyPosture(State& state, float deltaTime);
void resetReadyPosture(State& state);
void setReadyPosture(State& state, ReadyPosture posture, float durationSeconds);
void playDirectionalClip(game::SpriteAnimation& animation, game::AnimationState& animState,
                         bool facingLeft, const char* leftClipKey, const char* rightClipKey);
void playReadyPostureClip(game::SpriteAnimation& animation, game::AnimationState& animState, const State& state);
void enterStageEndPose(State& state, game::SpriteAnimation& animation, game::AnimationState& animState);

} // namespace core::huntercontroller
