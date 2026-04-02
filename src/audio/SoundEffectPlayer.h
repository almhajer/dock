#pragma once

#include <string>

namespace audio {

/// مشغل مؤثرات صوتية خفيف لعينات قصيرة مثل صوت إطلاق النار.
class SoundEffectPlayer {
public:
    SoundEffectPlayer() = default;
    ~SoundEffectPlayer();

    SoundEffectPlayer(const SoundEffectPlayer&) = delete;
    SoundEffectPlayer& operator=(const SoundEffectPlayer&) = delete;

    /// تحميل ملف صوتي قصير من القرص وإبقاؤه جاهزًا للتشغيل السريع.
    bool load(const std::string& path);

    /// تشغيل المؤثر إذا كان محملًا بنجاح.
    void play();

    /// تحرير المورد الصوتي الحالي بأمان.
    void reset();

    /// هل المؤثر الصوتي جاهز للتشغيل؟
    [[nodiscard]] bool isLoaded() const;

private:
    void* mState = nullptr;
};

} // namespace audio
