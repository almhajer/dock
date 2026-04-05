#pragma once

#include "SpriteAtlas.h"

namespace game {

/// يبني بيانات الأطلس الموحد للصياد (حركة + إطلاق + إطلاق عالي) من الصورة الجديدة.
[[nodiscard]] SpriteAtlasData createHunterAtlasData(int imageWidth, int imageHeight);

} // namespace game
