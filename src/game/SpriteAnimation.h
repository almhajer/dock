#pragma once

#include "SpriteAtlas.h"

#include <string>

namespace game
{

#pragma region StateTypes
/*
 حالة الحركة الحالية
 */
struct AnimationState
{
    /*
     اسم الكليب الجاري تشغيله.
     */
    std::string currentClip;

    /*
     مؤشر مباشر للكليب النشط لتسريع الوصول.
     */
    const AnimationClip* activeClip = nullptr;

    /*
     الزمن المتراكم داخل الكليب.
     */
    float elapsed = 0.0f; // الزمن المتراكم داخل الكليب بالثواني

    /*
     فهرس الفريم الحالي داخل الأطلس.
     */
    int currentFrameIndex = 0; // فهرس الفريم الحالي داخل data.frames

    /*
     هل انتهى الكليب غير الدائري؟
     */
    bool finished = false; // هل انتهى كليب غير دائري؟

    /*
     هل يجب عكس UV أفقياً؟
     */
    bool flipX = false; // هل يجب عكس UV أفقيًا عند الرسم؟
};
#pragma endregion StateTypes

/*
 نظام حركة السبرايت المعتمد على أطلس مجهّز مسبقًا.
 */
class SpriteAnimation
{
  public:
#pragma region PublicInterface
    /*
     ينشئ مشغل الأنيميشن بحالة فارغة.
     */
    SpriteAnimation() = default;

    /*
     تعيين بيانات أطلس جاهزة من مصدر خارجي
     */
    void setAtlasData(SpriteAtlasData data);

    /*
     تحديث الحالة حسب الزمن
     */
    void update(AnimationState& state, float deltaTime) const;

    /*
     بدء تشغيل مقطع حركة
     */
    void play(AnimationState& state, const std::string& clipKey) const;

    /*
     حساب إحداثيات UV للفريم الحالي
     */
    void getFrameUV(int frameIndex, float& u0, float& u1, float& v0, float& v1) const;

    /*
     الحصول على فريم محدد من atlas
     */
    const AtlasFrame* getFrame(int frameIndex) const;
#pragma endregion PublicInterface

  private:
#pragma region InternalHelpers
    /*
     يبحث عن كليب بالحقل النصي المفتاحي.
     */
    const AnimationClip* findClip(const std::string& key) const;
#pragma endregion InternalHelpers

#pragma region InternalState
    /*
     البيانات الكاملة للأطلس الجاري استخدامه.
     */
    SpriteAtlasData mData;
#pragma endregion InternalState
};

} // namespace game
