#pragma once

#include "SpriteAtlas.h"

#include <string>

namespace game {

/// حالة الحركة الحالية
struct AnimationState {
    std::string currentClip;
    const AnimationClip* activeClip = nullptr;
    float elapsed = 0.0f;      // الزمن المتراكم داخل الكليب بالثواني
    int currentFrameIndex = 0; // فهرس الفريم الحالي داخل data.frames
    bool finished = false;     // هل انتهى كليب غير دائري؟
    bool flipX = false;        // هل يجب عكس UV أفقيًا عند الرسم؟
};

/// نظام حركة السبرايت المعتمد على أطلس مجهّز مسبقًا.
class SpriteAnimation {
public:
    SpriteAnimation() = default;

    /// تعيين بيانات أطلس جاهزة من مصدر خارجي
    void setAtlasData(SpriteAtlasData data);

    /// تحديث الحالة حسب الزمن
    void update(AnimationState& state, float deltaTime) const;

    /// بدء تشغيل مقطع حركة
    void play(AnimationState& state, const std::string& clipKey) const;

    /// حساب إحداثيات UV للفريم الحالي
    void getFrameUV(int frameIndex, float& u0, float& u1, float& v0, float& v1) const;

    /// الحصول على فريم محدد من atlas
    const AtlasFrame* getFrame(int frameIndex) const;

private:
    const AnimationClip* findClip(const std::string& key) const;

    SpriteAtlasData mData;
};

} // namespace game
