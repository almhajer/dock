#pragma once

#include "SpriteAtlas.h"

#include <string>
#include <vector>

namespace game {

/// أطلس البطة النهائي داخل الذاكرة:
/// صورة RGBA جاهزة للرفع إلى GPU + بيانات الفريمات والأنيميشن.
struct DuckAtlasSheet {
    SpriteAtlasData atlas;
    std::vector<unsigned char> pixels;
    int imageWidth = 0;
    int imageHeight = 0;
};

/// يبني بيانات أطلس البطة الإنتاجي:
/// 24 فريم في شبكة 6×4، مقسمة إلى fly / hit / fall / dead.
[[nodiscard]] SpriteAtlasData createDuckAtlasData(int imageWidth, int imageHeight);

/// يبني أطلس البطة الإنتاجي من بكسلات الصورة الأصلية `duck.png`.
[[nodiscard]] DuckAtlasSheet createDuckAtlasSheetFromPixels(const unsigned char* sourcePixels,
                                                            int sourceWidth,
                                                            int sourceHeight);

/// يحمّل `duck.png` من القرص ثم يبني أطلس البطة النهائي بالكامل داخل C++.
[[nodiscard]] DuckAtlasSheet loadDuckAtlasSheetFromSourceImage(const std::string& sourceImagePath);

} // namespace game
