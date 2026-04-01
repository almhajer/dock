#include "SpriteAnimation.h"
#include "HunterSpriteAtlas.h"

#include <iostream>
#include <utility>

namespace game {
namespace {

constexpr float kTexelInset = 0.5f;

bool isLeftFacingClipKey(const std::string& clipKey)
{
    return clipKey.size() >= 5 && clipKey.compare(clipKey.size() - 5, 5, "_left") == 0;
}

void resetAnimationState(AnimationState& state, const std::string& clipKey)
{
    state.currentClip = clipKey;
    state.elapsed = 0.0f;
    state.currentFrameIndex = 0;
    state.finished = false;
}

} // namespace

// ─── بناء أطلس الصياد من البكسلات ─────────────────────────────────

void SpriteAnimation::setAtlasData(SpriteAtlasData data) {
    mData = std::move(data);
}

void SpriteAnimation::buildHunterAtlas(int imgW, int imgH, const unsigned char* pixels) {
    if (imgW <= 0 || imgH <= 0 || !pixels) {
        mData = SpriteAtlasData{};
        std::cerr << "[Sprite] بيانات صورة غير صالحة" << std::endl;
        return;
    }

    mData = createHunterSpriteAtlasData(imgW, imgH);

    std::cout << "[Sprite] تم تحميل atlas الصياد الثابتة: " << mData.frames.size()
              << " فريم (" << imgW << "x" << imgH << ")" << std::endl;
}

// ─── تحديث الحالة ────────────────────────────────────────────────

void SpriteAnimation::update(AnimationState& state, float deltaTime) const {
    const AnimationClip* clip = findClip(state.currentClip);
    if (clip == nullptr || clip->frames.empty()) return;

    state.elapsed += deltaTime;

    int rawIndex = static_cast<int>(state.elapsed * static_cast<float>(clip->fps));

    if (clip->loop) {
        rawIndex = rawIndex % static_cast<int>(clip->frames.size());
        state.finished = false;
    } else {
        if (rawIndex >= static_cast<int>(clip->frames.size())) {
            rawIndex = static_cast<int>(clip->frames.size()) - 1;
            state.finished = true;
        }
    }

    state.currentFrameIndex = clip->frames[rawIndex];
}

// ─── بدء تشغيل مقطع ─────────────────────────────────────────────

void SpriteAnimation::play(AnimationState& state, const std::string& clipKey) const {
    if (state.currentClip == clipKey && !state.finished) return;

    resetAnimationState(state, clipKey);

    const AnimationClip* clip = findClip(clipKey);
    if (clip != nullptr && !clip->frames.empty()) {
        state.currentFrameIndex = clip->frames[0];
        state.flipX = isLeftFacingClipKey(clipKey);
    }
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

    // سحب UV قليلًا للداخل لمنع sampler من قراءة بكسلات الفريم المجاور.
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
