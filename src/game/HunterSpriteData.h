#pragma once

namespace game {

/// توقيتات حركة الصياد المستخدمة فعليًا داخل التطبيق.
struct HunterActionTiming {
    float reloadDurationSeconds = 0.0f;
};

/// الوصول إلى توقيتات الإطلاق وإعادة التعبئة.
[[nodiscard]] const HunterActionTiming& hunterActionTiming();

} // namespace game
