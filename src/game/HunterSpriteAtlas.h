#pragma once

#include "SpriteAtlas.h"

namespace game {

/// يبني بيانات أطلس الحركة الأساسية الخاصة بالصياد من ملف البيانات الثابتة.
[[nodiscard]] SpriteAtlasData createHunterSpriteAtlasData(int imageWidth, int imageHeight);

/// يبني بيانات أطلس إطلاق النار الخاصة بالصياد من ملف البيانات الثابتة.
[[nodiscard]] SpriteAtlasData createHunterShootSpriteAtlasData(int imageWidth, int imageHeight);

/// يبني بيانات أطلس إطلاق النار العالي الخاصة بالصياد من ملف البيانات الثابتة.
[[nodiscard]] SpriteAtlasData createHunterHighShootSpriteAtlasData(int imageWidth, int imageHeight);

} // namespace game
