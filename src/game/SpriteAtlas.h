#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace game {

/// فريم واحد داخل أطلس السبرايت
struct AtlasFrame {
    int x = 0;             // بداية الفريم أفقيًا داخل الصورة
    int y = 0;             // بداية الفريم عموديًا داخل الصورة
    int width = 0;         // عرض الفريم المقصوص
    int height = 0;        // ارتفاع الفريم المقصوص
    int offsetX = 0;       // إزاحة المحتوى داخل الخلية المنطقية
    int offsetY = 0;       // إزاحة المحتوى داخل الخلية المنطقية
    int sourceWidth = 0;   // العرض المنطقي الثابت للخلية
    int sourceHeight = 0;  // الارتفاع المنطقي الثابت للخلية
};

/// مقطع حركة مثل: walk_right, idle_left
struct AnimationClip {
    std::string key;
    std::string labelAr;
    std::string labelEn;
    std::string type;       // walk, idle
    std::string direction;  // left, right
    std::vector<int> frames;
    int fps = 8;
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
