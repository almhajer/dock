#pragma once

#include <span>

namespace game {

/// مستطيل خام يُستخدم لتعريف مواقع الفريمات داخل الصورة المصدر.
struct RawAtlasFrame {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

/// إعدادات أطلس الحركة الأساسية للصياد في sprite.png.
struct HunterMoveAtlasConfig {
    int imageWidth = 0;
    int imageHeight = 0;
    int columns = 0;
    int cellWidth = 0;
    int cellHeight = 0;
    int gutter = 0;
    std::span<const RawAtlasFrame> frames;
    std::span<const int> walkFrames;
    std::span<const int> idleFrames;
};

/// إعدادات أطلس الإطلاق للصياد في hunterGun.png.
struct HunterShootAtlasConfig {
    int imageWidth = 0;
    int imageHeight = 0;
    int sourceWidth = 0;
    int sourceHeight = 0;
    std::span<const RawAtlasFrame> frames;
    std::span<const int> shootFrames;
    std::span<const int> shootRecoverFrames;
    std::span<const int> shootReadyFrames;
};

/// توقيتات الانتقال الخاصة بإطلاق النار وإعادة التعبئة.
struct HunterActionTiming {
    float reloadDurationSeconds = 0.0f;
    float shootRecoverHoldSeconds = 0.0f;
    float shootReadySettleSeconds = 0.0f;
};

/// الوصول إلى إعدادات أطلس الحركة الأساسية من ملف بيانات واحد.
[[nodiscard]] const HunterMoveAtlasConfig& hunterMoveAtlasConfig();

/// الوصول إلى إعدادات أطلس الإطلاق من ملف بيانات واحد.
[[nodiscard]] const HunterShootAtlasConfig& hunterShootAtlasConfig();

/// الوصول إلى توقيتات الإطلاق وإعادة التعبئة بدل نشرها داخل App.cpp.
[[nodiscard]] const HunterActionTiming& hunterActionTiming();

} // namespace game
