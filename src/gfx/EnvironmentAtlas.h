#pragma once

#include "EnvironmentTypes.h"

#include <vector>

namespace gfx {

struct EnvironmentAtlasBitmap {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
};

/// توليد atlas إجرائية مشتركة للغيوم والأشجار لتقليل عدد الخامات والـ draw calls.
[[nodiscard]] EnvironmentAtlasBitmap createEnvironmentAtlasBitmap();

/// الحصول على UV الخاصة بعنصر محدد داخل atlas.
[[nodiscard]] UvRect environmentAtlasUv(EnvironmentSpriteId spriteId);

/// نوع العنصر المرسوم داخل atlas.
[[nodiscard]] EnvironmentElementKind environmentSpriteKind(EnvironmentSpriteId spriteId);

} // namespace gfx
