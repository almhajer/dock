#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace game {

/// فريم واحد داخل أطلس السبرايت
struct AtlasFrame {
    int x = 0;         // بداية الخلية أفقيًا (بكسل)
    int y = 0;         // بداية الخلية عموديًا (بكسل)
    int width = 0;     // عرض الخلية (بكسل)
    int height = 0;    // ارتفاع الخلية (بكسل)
    int offsetX = 0;   // إزاحة المحتوى داخل الخلية المنطقية
    int offsetY = 0;   // إزاحة المحتوى داخل الخلية المنطقية
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

/// حالة الحركة الحالية
struct AnimationState {
    std::string currentClip;
    float elapsed = 0.0f;
    int currentFrameIndex = 0;
    bool finished = false;
    bool flipX = false;
};

/// نظام حركة السبرايت - يبني بيانات الأطلس من atlas معروفة مسبقًا
class SpriteAnimation {
public:
    SpriteAnimation() = default;

    /// بناء بيانات أطلس الصياد من metadata ثابتة متوافقة مع sprite.png
    void buildHunterAtlas(int imgW, int imgH, const unsigned char* pixels);

    /// تحديث الحالة حسب الزمن
    void update(AnimationState& state, float deltaTime) const;

    /// بدء تشغيل مقطع حركة
    void play(AnimationState& state, const std::string& clipKey) const;

    /// حساب إحداثيات UV للفريم الحالي
    void getFrameUV(int frameIndex, float& u0, float& u1, float& v0, float& v1) const;

    /// الحصول على مقطع حركة بالاسم
    const AnimationClip* getClip(const std::string& key) const;

    /// بيانات الأطلس
    const SpriteAtlasData& getData() const { return mData; }

private:
    SpriteAtlasData mData;
};

} // namespace game
