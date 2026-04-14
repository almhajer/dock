#include "HunterSpriteData.h"

namespace game
{
namespace
{

const HunterActionTiming HUNTER_ACTION_TIMING = {
    .reloadDurationSeconds = 0.65f,
    .readyPostureHoldDurationSeconds = 1.0f,
};

} // namespace

const HunterActionTiming& hunterActionTiming()
{
    return HUNTER_ACTION_TIMING;
}

} // namespace game
