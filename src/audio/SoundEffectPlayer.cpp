#include "SoundEffectPlayer.h"

#include "third_party/miniaudio.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <new>

namespace audio {
namespace {

constexpr float DEFAULT_EFFECT_VOLUME = 0.42f;

/// حالة التشغيل الفعلية مخفية عن الترويسة حتى يبقى المشغل C++ عاديًا.
struct PlayerState {
    ma_engine engine {};
    ma_sound sound {};
    bool engineInitialized = false;
    bool soundInitialized = false;
};

PlayerState* stateFromHandle(void* handle)
{
    return static_cast<PlayerState*>(handle);
}

void cleanupState(PlayerState& state)
{
    if (state.soundInitialized)
    {
        ma_sound_uninit(&state.sound);
        state.soundInitialized = false;
    }

    if (state.engineInitialized)
    {
        ma_engine_uninit(&state.engine);
        state.engineInitialized = false;
    }
}

} // namespace

SoundEffectPlayer::~SoundEffectPlayer()
{
    reset();
}

bool SoundEffectPlayer::load(const std::string& path)
{
    reset();

    const std::filesystem::path resolvedPath = std::filesystem::absolute(std::filesystem::path(path));
    const std::string resolvedPathString = resolvedPath.string();

    if (!std::filesystem::exists(resolvedPath))
    {
        std::cerr << "[Audio] ملف المؤثر غير موجود: " << resolvedPathString << std::endl;
        return false;
    }

    std::unique_ptr<PlayerState> state = std::make_unique<PlayerState>();

    // نستخدم محركًا واحدًا خفيفًا لكل مؤثر حتى يبقى التشغيل المباشر بسيطًا وواضحًا.
    const ma_result engineResult = ma_engine_init(nullptr, &state->engine);
    if (engineResult != MA_SUCCESS)
    {
        std::cerr << "[Audio] تعذر تهيئة محرك الصوت للمؤثر: " << resolvedPathString
                  << " (ma_result=" << engineResult << ")" << std::endl;
        return false;
    }
    state->engineInitialized = true;

    const ma_result soundResult = ma_sound_init_from_file(
        &state->engine,
        resolvedPathString.c_str(),
        MA_SOUND_FLAG_DECODE,
        nullptr,
        nullptr,
        &state->sound);
    if (soundResult != MA_SUCCESS)
    {
        std::cerr << "[Audio] تعذر تحميل المؤثر الصوتي: " << resolvedPathString
                  << " (ma_result=" << soundResult << ")" << std::endl;
        cleanupState(*state);
        return false;
    }
    state->soundInitialized = true;

    // نخفض شدة المؤثر افتراضيًا حتى لا يطغى صوت الإطلاق على بقية المشهد.
    ma_sound_set_volume(&state->sound, DEFAULT_EFFECT_VOLUME);
    mState = state.release();
    return true;
}

void SoundEffectPlayer::play()
{
    PlayerState* state = stateFromHandle(mState);
    if (state == nullptr || !state->soundInitialized)
    {
        return;
    }

    // نوقف المؤثر ونرجعه للبداية حتى تتطابق كل ضغطة إطلاق مع بداية الصوت.
    ma_sound_stop(&state->sound);
    ma_sound_seek_to_pcm_frame(&state->sound, 0);
    ma_sound_start(&state->sound);
}

void SoundEffectPlayer::reset()
{
    std::unique_ptr<PlayerState> state(stateFromHandle(mState));
    if (!state)
    {
        return;
    }

    cleanupState(*state);
    mState = nullptr;
}

bool SoundEffectPlayer::isLoaded() const
{
    const PlayerState* state = static_cast<const PlayerState*>(mState);
    return state != nullptr && state->soundInitialized;
}

} // namespace audio
