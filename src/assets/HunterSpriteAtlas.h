/**
 * @file HunterSpriteAtlas.h
 * @brief واجهة أطلس الصياد.
 * @details بناء بيانات الأطلس للصياد.
 */

#pragma once

#include "SpriteAtlas.h"

namespace game
{

#pragma region AtlasBuilders
/*
 يبني بيانات الأطلس الموحد للصياد (حركة + إطلاق + إطلاق عالي) من الصورة الجديدة.
 */
[[nodiscard]] SpriteAtlasData createHunterAtlasData(int imageWidth, int imageHeight);
#pragma endregion AtlasBuilders

} // namespace game
