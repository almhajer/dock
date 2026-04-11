#pragma once

#include "EnvironmentTypes.h"

#include <vector>

namespace gfx {

#pragma region AtlasTypes
/// @brief صورة أطلس بيئة جاهزة للرفع إلى GPU.
struct EnvironmentAtlasBitmap {
    /// @brief عرض الصورة الناتجة.
    int width = 0;
    /// @brief ارتفاع الصورة الناتجة.
    int height = 0;
    /// @brief البكسلات الخام بنمط RGBA.
    std::vector<unsigned char> pixels;
};
#pragma endregion AtlasTypes

#pragma region AtlasQueries
/// توليد atlas إجرائية مشتركة للغيوم والأشجار لتقليل عدد الخامات والـ draw calls.
[[nodiscard]] EnvironmentAtlasBitmap createEnvironmentAtlasBitmap();

/// الحصول على UV الخاصة بعنصر محدد داخل atlas.
[[nodiscard]] UvRect environmentAtlasUv(EnvironmentSpriteId spriteId);

/// نوع العنصر المرسوم داخل atlas.
[[nodiscard]] EnvironmentElementKind environmentSpriteKind(EnvironmentSpriteId spriteId);
#pragma endregion AtlasQueries

} // namespace gfx
