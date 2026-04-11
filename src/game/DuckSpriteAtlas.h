#pragma once

#include "SpriteAtlas.h"

#include <string>
#include <vector>

namespace game
{

#pragma region AtlasTypes
/*
 أطلس البطة النهائي داخل الذاكرة:
 صورة RGBA جاهزة للرفع إلى GPU + بيانات الفريمات والأنيميشن.
 */
struct DuckAtlasSheet
{
    /*
     وصف الأطلس والفريمات والكليبات الناتجة.
     */
    SpriteAtlasData atlas;

    /*
     البكسلات الخام الجاهزة للرفع إلى GPU.
     */
    std::vector<unsigned char> pixels;

    /*
     عرض الصورة النهائية للأطلس.
     */
    int imageWidth = 0;

    /*
     ارتفاع الصورة النهائية للأطلس.
     */
    int imageHeight = 0;
};
#pragma endregion AtlasTypes

#pragma region AtlasBuilders
/*
 يبني بيانات أطلس البطة الإنتاجي:
 11 فريم مرتبة إلى fly (أول 6) ثم hit (آخر 5).
 */
[[nodiscard]] SpriteAtlasData createDuckAtlasData(int imageWidth, int imageHeight,
                                                  const std::vector<AtlasFrame>& frames);

/*
 يبني أطلس البطة الإنتاجي من بكسلات الصورة الأصلية `duck.png`.
 */
[[nodiscard]] DuckAtlasSheet createDuckAtlasSheetFromPixels(const unsigned char* sourcePixels, int sourceWidth,
                                                            int sourceHeight);

/*
 يحمّل `duck.png` من القرص ويستخدمه كمصدر ومرجع
 لبناء أطلس البطة النهائي بدون الاعتماد على مجلد خارجي.
 */
[[nodiscard]] DuckAtlasSheet loadDuckAtlasSheetFromSourceImage(const std::string& sourceImagePath);
#pragma endregion AtlasBuilders

} // namespace game
