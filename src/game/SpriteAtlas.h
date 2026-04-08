#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace game {

/// فريم واحد داخل أطلس السبرايت
///
/// نظام الإحداثيات: top-left (الأصل في الزاوية العلوية اليسرى).
///
/// العلاقة بين الحقول:
///   frame (x,y,width,height)     → المستطيل المقصوص داخل صورة الأطلس
///   sourceSize (sourceW,sourceH) → الحجم الأصلي للوحة المرجعية قبل القص
///   spriteSourceSize (spriteX,spriteY) → إزاحة الفريم المقصوص داخل اللوحة المرجعية
///   pivot (pivotX,pivotY)        → نقطة الارتكاز (0-1) داخل اللوحة المرجعية
///
/// مثال: شخصية بحجم أصلي 220×320، الفريم المقصوص يبدأ عند (35,12) داخل اللوحة:
///   sourceW=220, sourceH=320, spriteX=35, spriteY=12, width=150, height=296
///   pivot (0.5, 0.9) = منتصف الأسفل (قدمين)
struct AtlasFrame {
    int x = 0;             // بداية الفريم أفقيًا داخل صورة الأطلس (frame.x)
    int y = 0;             // بداية الفريم عموديًا داخل صورة الأطلس (frame.y)
    int width = 0;         // عرض الفريم المقصوص (frame.w)
    int height = 0;        // ارتفاع الفريم المقصوص (frame.h)
    int sourceW = 0;       // عرض اللوحة المرجعية الأصلية (sourceSize.w)
    int sourceH = 0;       // ارتفاع اللوحة المرجعية الأصلية (sourceSize.h)
    int spriteX = 0;       // إزاحة الفريم المقصوص أفقيًا داخل اللوحة (spriteSourceSize.x)
    int spriteY = 0;       // إزاحة الفريم المقصوص عموديًا داخل اللوحة (spriteSourceSize.y)
    float pivotX = 0.5f;   // نقطة الارتكاز الأفقية داخل اللوحة (0=يسار, 1=يمين)
    float pivotY = 1.0f;   // نقطة الارتكاز العمودية داخل اللوحة (0=أعلى, 1=أسفل)
    int durationMs = 80;   // مدة عرض الفريم بالملي ثانية
};

/// مقطع حركة مثل: walk_right, idle_left
struct AnimationClip {
    std::string key;
    std::vector<int> frames;
    bool loop = true;
};

/// بيانات أطلس السبرايت الكاملة
struct SpriteAtlasData {
    int imageWidth = 0;
    int imageHeight = 0;
    std::vector<AtlasFrame> frames;
    std::unordered_map<std::string, AnimationClip> animations;
};

} // namespace game
