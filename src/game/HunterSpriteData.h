#pragma once

namespace game {

/// توقيتات الانتقال الخاصة بإطلاق النار وإعادة التعبئة.
struct HunterActionTiming {
    float reloadDurationSeconds = 0.0f;
    float shootRecoverHoldSeconds = 0.0f;
    float highShootReadyHoldSeconds = 0.0f;
    float shootReadySettleSeconds = 0.0f;
};

/// الوصول إلى توقيتات الإطلاق وإعادة التعبئة.
[[nodiscard]] const HunterActionTiming& hunterActionTiming();

} // namespace game
