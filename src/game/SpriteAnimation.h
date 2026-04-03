#pragma once

#include "SpriteAtlas.h"

#include <string>

namespace game {

/// حالة الحركة الحالية
struct AnimationState {
    std::string currentClip;
    const AnimationClip* activeClip = nullptr;
    float elapsed = 0.0f;
    int currentFrameIndex = 0;
    bool finished = false;
    bool flipX = false;
};

/// نظام حركة السبرايت - يبني بيانات الأطلس من atlas معروفة مسبقًا
class SpriteAnimation {
public:
    SpriteAnimation() = default;

    /// تعيين بيانات أطلس جاهزة من مصدر خارجي
    void setAtlasData(SpriteAtlasData data);

    /// بناء بيانات أطلس الصياد من metadata ثابتة متوافقة مع sprite.png
    void buildHunterAtlas(int imgW, int imgH, const unsigned char* pixels);

    /// تحديث الحالة حسب الزمن
    void update(AnimationState& state, float deltaTime) const;

    /// بدء تشغيل مقطع حركة
    void play(AnimationState& state, const std::string& clipKey) const;

    /// حساب إحداثيات UV للفريم الحالي
    void getFrameUV(int frameIndex, float& u0, float& u1, float& v0, float& v1) const;

    /// الحصول على فريم محدد من atlas
    const AtlasFrame* getFrame(int frameIndex) const;

    /// الحصول على مقطع حركة بالاسم
    const AnimationClip* getClip(const std::string& key) const;

    /// بيانات الأطلس
    const SpriteAtlasData& getData() const { return mData; }

private:
    const AnimationClip* findClip(const std::string& key) const;

    SpriteAtlasData mData;
};

} // namespace game
