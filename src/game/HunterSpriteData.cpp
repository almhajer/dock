#include "HunterSpriteData.h"

namespace game {
namespace {

const HunterActionTiming HUNTER_ACTION_TIMING = {
    .reloadDurationSeconds = 0.65f,
    .shootRecoverHoldSeconds = 3.0f,
    .highShootReadyHoldSeconds = 1.25f,
    .shootReadySettleSeconds = 0.22f,
};

} // namespace

const HunterActionTiming& hunterActionTiming()
{
    return HUNTER_ACTION_TIMING;
}

} // namespace game
