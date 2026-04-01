#pragma once

#include "SpriteAtlas.h"

namespace game {

/// يبني بيانات atlas الخاصة بالصياد من metadata ثابتة متوافقة مع sprite.png الحالية.
[[nodiscard]] SpriteAtlasData createHunterSpriteAtlasData(int imageWidth, int imageHeight);

} // namespace game
