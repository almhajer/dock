#include "SpriteAnimation.h"

#include <utility>

namespace game {
namespace {

constexpr float kTexelInset = 0.5f;

bool isLeftFacingClipKey(const std::string& clipKey)
{
    return clipKey.size() >= 5 && clipKey.compare(clipKey.size() - 5, 5, "_left") == 0;
}

void resetAnimationState(AnimationState& state,
                         const std::string& clipKey,
                         const AnimationClip* clip)
{
    state.currentClip = clipKey;
    state.activeClip = clip;
    state.elapsed = 0.0f;
    state.currentFrameIndex = 0;
    state.finished = false;
}

/// حساب الفريم الحالي بناءً على المدة التراكمية لكل فريم.
/// تُرجع فهرس الفريم داخل الكليب، أو -1 إذا تجاوز إجمالي المدة.
int resolveFrameByDuration(const SpriteAtlasData& data,
                           const AnimationClip& clip,
                           float elapsedMs)
{
    float accumulated = 0.0f;
    const int count = static_cast<int>(clip.frames.size());
    for (int i = 0; i < count; ++i)
    {
        const AtlasFrame& frame = data.frames[clip.frames[i]];
        accumulated += static_cast<float>(frame.durationMs);
        if (elapsedMs < accumulated)
        {
            return i;
        }
    }
    return -1; // تجاوز إجمالي المدة
}

/// حساب إجمالي مدة الكليب بالملي ثانية.
float totalClipDurationMs(const SpriteAtlasData& data,
                          const AnimationClip& clip)
{
    float total = 0.0f;
    for (int idx : clip.frames)
    {
        total += static_cast<float>(data.frames[idx].durationMs);
    }
    return total;
}

} // namespace

// ─── تعيين بيانات الأطلس ──────────────────────────────────────────

void SpriteAnimation::setAtlasData(SpriteAtlasData data) {
    mData = std::move(data);
}

// ─── تحديث الحالة ────────────────────────────────────────────────

void SpriteAnimation::update(AnimationState& state, float deltaTime) const {
    const AnimationClip* clip = state.activeClip;
    if (clip == nullptr)
    {
        clip = findClip(state.currentClip);
        state.activeClip = clip;
    }
    if (clip == nullptr || clip->frames.empty()) return;

    state.elapsed += deltaTime;

    const float elapsedMs = state.elapsed * 1000.0f;
    const int clipLen = static_cast<int>(clip->frames.size());

    int rawIndex = resolveFrameByDuration(mData, *clip, elapsedMs);

    if (clip->loop)
    {
        if (rawIndex < 0)
        {
            // لف الحلقة: حساب الموضع داخل الدورة التالية
            const float totalMs = totalClipDurationMs(mData, *clip);
            if (totalMs > 0.0f)
            {
                const float wrappedMs = std::fmod(elapsedMs, totalMs);
                rawIndex = resolveFrameByDuration(mData, *clip, wrappedMs);
                if (rawIndex < 0) rawIndex = clipLen - 1;
            }
            else
            {
                rawIndex = 0;
            }
        }
        state.finished = false;
    }
    else
    {
        if (rawIndex < 0)
        {
            rawIndex = clipLen - 1;
            state.finished = true;
        }
    }

    state.currentFrameIndex = clip->frames[rawIndex];
}

// ─── بدء تشغيل مقطع ─────────────────────────────────────────────

void SpriteAnimation::play(AnimationState& state, const std::string& clipKey) const {
    const AnimationClip* clip = findClip(clipKey);
    if (state.currentClip == clipKey && !state.finished && state.activeClip == clip) return;

    if (clip == nullptr || clip->frames.empty())
    {
        state.currentClip.clear();
        state.activeClip = nullptr;
        state.elapsed = 0.0f;
        state.currentFrameIndex = -1;
        state.finished = true;
        return;
    }

    resetAnimationState(state, clipKey, clip);

    state.currentFrameIndex = clip->frames[0];
    state.flipX = isLeftFacingClipKey(clipKey);
}

// ─── حساب UV للفريم ─────────────────────────────────────────────

void SpriteAnimation::getFrameUV(int frameIndex, float& u0, float& u1, float& v0, float& v1) const {
    const AtlasFrame* frame = getFrame(frameIndex);
    if (frame == nullptr) {
        u0 = u1 = v0 = v1 = 0.0f;
        return;
    }

    float imgW = static_cast<float>(mData.imageWidth);
    float imgH = static_cast<float>(mData.imageHeight);

    const float insetX = (frame->width > 1) ? kTexelInset : 0.0f;
    const float insetY = (frame->height > 1) ? kTexelInset : 0.0f;

    u0 = (static_cast<float>(frame->x) + insetX) / imgW;
    u1 = (static_cast<float>(frame->x + frame->width) - insetX) / imgW;
    v0 = (static_cast<float>(frame->y) + insetY) / imgH;
    v1 = (static_cast<float>(frame->y + frame->height) - insetY) / imgH;
}

const AtlasFrame* SpriteAnimation::getFrame(int frameIndex) const {
    if (frameIndex < 0 || frameIndex >= static_cast<int>(mData.frames.size())) {
        return nullptr;
    }
    return &mData.frames[frameIndex];
}

// ─── الحصول على مقطع ────────────────────────────────────────────

const AnimationClip* SpriteAnimation::getClip(const std::string& key) const {
    return findClip(key);
}

const AnimationClip* SpriteAnimation::findClip(const std::string& key) const {
    auto it = mData.animations.find(key);
    return (it != mData.animations.end()) ? &it->second : nullptr;
}

} // namespace game
