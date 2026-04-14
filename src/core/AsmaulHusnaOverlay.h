#pragma once

#include "../gfx/RenderTypes.h"
#include "../audio/SoundEffectPlayer.h"

#include <string>
#include <vector>

namespace core
{

/*
 * تراكب أسماء الله الحسنى:
 * - الصوت يُشغّل مع كل إطلاق نار
 * - النص يظهر عند مكان إصابة البطة مع خلفية شفافة ويتلاشى خلال ثانيتين
 */
class AsmaulHusnaOverlay
{
  public:
#pragma region PublicTypes
    struct Entry
    {
        std::string meaningAr;
        std::string audioPath;
    };

    struct GlyphUV
    {
        float u0 = 0.0f;
        float u1 = 0.0f;
        float v0 = 0.0f;
        float v1 = 0.0f;
        int pixelWidth = 0;
        int pixelHeight = 0;
    };
#pragma endregion PublicTypes

#pragma region Lifecycle
    AsmaulHusnaOverlay() = default;
    ~AsmaulHusnaOverlay() = default;
    AsmaulHusnaOverlay(const AsmaulHusnaOverlay&) = delete;
    AsmaulHusnaOverlay& operator=(const AsmaulHusnaOverlay&) = delete;

    bool initialize(const std::string& assetsPath);
#pragma endregion Lifecycle

#pragma region Gameplay
    /*
     يُستدعى مع كل إطلاق نار — يشغّل المقطع الصوتي ويقدم المؤشر.
     */
    void onShot(audio::SoundEffectPlayer& primary, audio::SoundEffectPlayer& secondary, const std::string& assetsPath);

    /*
     يُستدعى عند إصابة بطة — يعرض المعنى عند الموضع المحدد.
     @param screenX الإحداثي الأفقي المعياري للشاشة.
     @param screenY الإحداثي العمودي المعياري للشاشة.
     */
    void onHit(float screenX, float screenY);

    void update(float deltaTime, audio::SoundEffectPlayer& primary, audio::SoundEffectPlayer& secondary);
    [[nodiscard]] bool isActive() const;

    [[nodiscard]] int currentIndex() const;
    void setCurrentIndex(int index);
#pragma endregion Gameplay

#pragma region RenderData
    [[nodiscard]] const std::vector<unsigned char>& texturePixels() const;
    [[nodiscard]] int textureWidth() const;
    [[nodiscard]] int textureHeight() const;

    void buildTextQuads(std::vector<gfx::TexturedQuad>& quads, float aspect) const;
    void buildBgQuad(std::vector<gfx::TexturedQuad>& quads, float aspect) const;
#pragma endregion RenderData

  private:
#pragma region InternalState
    std::vector<Entry> mEntries;
    std::vector<GlyphUV> mGlyphs;
    std::vector<unsigned char> mPixels;
    int mTexW = 0;
    int mTexH = 0;

    int mCurrentIndex = 0;
    int mActiveEntry = -1;
    std::vector<int> mPendingAudio;
    int mNextPreLoaded = -1;
    bool mPlayingSecondary = false;
    float mTimer = 0.0f;
    float mScreenX = 0.0f;
    float mScreenY = 0.0f;

    void preLoadNext(audio::SoundEffectPlayer& primary, audio::SoundEffectPlayer& secondary);

    static constexpr float kFadeDuration = 4.5f;
    static constexpr float kScreenH = 0.040f;
    static constexpr float kBgPadX = 0.012f;
    static constexpr float kBgPadY = 0.008f;
    static constexpr float kMargin = 0.02f;
#pragma endregion InternalState
};

} // namespace core
