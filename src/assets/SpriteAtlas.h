/**
 * @file SpriteAtlas.h
 * @brief واجهة أطلس السبرايت.
 * @details هياكل البيانات للأطالس والفريمات.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace game
{

#pragma region AtlasTypes
/*
 فريم واحد داخل أطلس السبرايت

 نظام الإحداثيات: top-left (الأصل في الزاوية العلوية اليسرى).

 العلاقة بين الحقول:
   frame (x,y,width,height)     → المستطيل المقصوص داخل صورة الأطلس
   sourceSize (sourceW,sourceH) → الحجم الأصلي للوحة المرجعية قبل القص
   spriteSourceSize (spriteX,spriteY) → إزاحة الفريم المقصوص داخل اللوحة المرجعية
   pivot (pivotX,pivotY)        → نقطة الارتكاز (0-1) داخل اللوحة المرجعية

 مثال: شخصية بحجم أصلي 220×320، الفريم المقصوص يبدأ عند (35,12) داخل اللوحة:
   sourceW=220, sourceH=320, spriteX=35, spriteY=12, width=150, height=296
   pivot (0.5, 0.9) = منتصف الأسفل (قدمين)
 */
struct AtlasFrame
{
    int x = 0;           // بداية الفريم أفقيًا داخل صورة الأطلس (frame.x)
    int y = 0;           // بداية الفريم عموديًا داخل صورة الأطلس (frame.y)
    int width = 0;       // عرض الفريم المقصوص (frame.w)
    int height = 0;      // ارتفاع الفريم المقصوص (frame.h)
    int sourceW = 0;     // عرض اللوحة المرجعية الأصلية (sourceSize.w)
    int sourceH = 0;     // ارتفاع اللوحة المرجعية الأصلية (sourceSize.h)
    int spriteX = 0;     // إزاحة الفريم المقصوص أفقيًا داخل اللوحة (spriteSourceSize.x)
    int spriteY = 0;     // إزاحة الفريم المقصوص عموديًا داخل اللوحة (spriteSourceSize.y)
    float pivotX = 0.5f; // نقطة الارتكاز الأفقية داخل اللوحة (0=يسار, 1=يمين)
    float pivotY = 1.0f; // نقطة الارتكاز العمودية داخل اللوحة (0=أعلى, 1=أسفل)
    int durationMs = 80; // مدة عرض الفريم بالملي ثانية
    float visualOffsetXPx = 0.0f; // تعويض بصري اختياري داخل اللوحة المرجعية بالبكسل
    float visualOffsetYPx = 0.0f; // تعويض بصري اختياري لتثبيت الوقفة أو baseline بين الفريمات
};

/*
 مقطع حركة مثل: walk_right, idle_left
 */
struct AnimationClip
{
    /*
     اسم الكليب.
     */
    std::string key;

    /*
     فهارس الفريمات التابعة للكليب.
     */
    std::vector<int> frames;

    /*
     هل الكليب دائري؟
     */
    bool loop = true;

    /*
     مجموع مدد فريمات الكليب.
     */
    int totalDurationMs = 0; // مجموع مدد الفريمات داخل الكليب
};

/*
 بيانات أطلس السبرايت الكاملة
 */
struct SpriteAtlasData
{
    /*
     عرض صورة الأطلس.
     */
    int imageWidth = 0;

    /*
     ارتفاع صورة الأطلس.
     */
    int imageHeight = 0;

    /*
     جميع الفريمات المتاحة داخل الأطلس.
     */
    std::vector<AtlasFrame> frames;

    /*
     جميع الكليبات المعرفة فوق الفريمات.
     */
    std::unordered_map<std::string, AnimationClip> animations;
};
#pragma endregion AtlasTypes

} // namespace game
