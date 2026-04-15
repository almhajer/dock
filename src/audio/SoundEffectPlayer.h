/**
 * @file SoundEffectPlayer.h
 * @brief واجهة مشغل المؤثرات الصوتية.
 * @details مشغل خفيف لعينات قصيرة مثل صوت إطلاق النار.
 */

#pragma once

#include <string>

namespace audio
{

/**
 * @brief مشغل مؤثرات صوتية خفيف لعينات قصيرة مثل صوت إطلاق النار.
 */
class SoundEffectPlayer
{
  public:
#pragma region PublicInterface
    /**
     * @brief ينشئ المشغل بحالة فارغة جاهزة للتحميل اللاحق.
     */
    SoundEffectPlayer() = default;

    /**
     * @brief يحرر كل الموارد الصوتية المرتبطة بالمشغل.
     */
    ~SoundEffectPlayer();

    /**
     * @brief يمنع نسخ المشغل لأن المورد الصوتي مملوك حصرياً للكائن.
     */
    SoundEffectPlayer(const SoundEffectPlayer&) = delete;

    /**
     * @brief يمنع إسناد النسخ لنفس سبب الملكية الحصرية.
     */
    SoundEffectPlayer& operator=(const SoundEffectPlayer&) = delete;

    /**
     * @brief تحميل ملف صوتي قصير من القرص وإبقاؤه جاهزًا للتشغيل السريع.
     * @param path مسار ملف الصوت.
     * @return true إذا نجح التحميل، false خلاف ذلك.
     */
    bool load(const std::string& path);

    /**
     * @brief تشغيل المؤثر إذا كان محملًا بنجاح.
     */
    void play();

    /**
     * @brief تشغيل المؤثر على حلقة مستمرة حتى يتم إيقافه يدويًا.
     */
    void playLooped();

    /**
     * @brief إيقاف التشغيل الحالي مع الإبقاء على الملف محملًا.
     */
    void stop();

    /*
     تحرير المورد الصوتي الحالي بأمان.
     */
    void reset();

    /*
     تغيير شدة الصوت (0.0 = صامت، 1.0 = كامل).
     */
    void setVolume(float volume);

    /*
     هل المؤثر الصوتي جاهز للتشغيل؟
     */
    [[nodiscard]] bool isLoaded() const;

    /*
     هل المؤثر يعمل حاليًا؟
     */
    [[nodiscard]] bool isPlaying() const;
#pragma endregion PublicInterface

  private:
#pragma region InternalState
    /*
     @brief مقبض داخلي إلى حالة miniaudio المخفية عن الترويسة.
     */
    void* mState = nullptr;
#pragma endregion InternalState
};

} // namespace audio
