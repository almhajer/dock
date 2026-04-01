#include "SpriteAnimation.h"

#include <algorithm>
#include <iostream>

namespace game {

namespace {

constexpr int HUNTER_ATLAS_IMAGE_WIDTH = 1176;
constexpr int HUNTER_ATLAS_IMAGE_HEIGHT = 1000;
constexpr int HUNTER_ATLAS_COLUMNS = 4;
constexpr int HUNTER_ATLAS_CELL_WIDTH = 273;
constexpr int HUNTER_ATLAS_CELL_HEIGHT = 486;
constexpr int HUNTER_ATLAS_GUTTER = 28;
constexpr AtlasFrame HUNTER_ATLAS_FRAMES[] = {
    {19, 11, 235, 465},
    {311, 11, 253, 465},
    {614, 10, 249, 466},
    {923, 17, 232, 459},
    {23, 531, 227, 459},
    {322, 534, 231, 456},
    {622, 534, 233, 456},
    {923, 528, 233, 462},
};

constexpr int HUNTER_ATLAS_FRAME_COUNT =
    static_cast<int>(sizeof(HUNTER_ATLAS_FRAMES) / sizeof(HUNTER_ATLAS_FRAMES[0]));

} // namespace

// ─── بناء أطلس الصياد من البكسلات ─────────────────────────────────

void SpriteAnimation::buildHunterAtlas(int imgW, int imgH, const unsigned char* pixels) {
    mData = SpriteAtlasData{};

    if (imgW <= 0 || imgH <= 0 || !pixels) {
        std::cerr << "[Sprite] بيانات صورة غير صالحة" << std::endl;
        return;
    }

    mData.imageWidth = imgW;
    mData.imageHeight = imgH;

    if (imgW != HUNTER_ATLAS_IMAGE_WIDTH || imgH != HUNTER_ATLAS_IMAGE_HEIGHT) {
        std::cerr << "[Sprite] تحذير: أبعاد الصورة لا تطابق atlas المتوقع ("
                  << HUNTER_ATLAS_IMAGE_WIDTH << "x" << HUNTER_ATLAS_IMAGE_HEIGHT
                  << "), الحالي " << imgW << "x" << imgH << std::endl;
    }

    mData.frames.reserve(HUNTER_ATLAS_FRAME_COUNT);
    for (int i = 0; i < HUNTER_ATLAS_FRAME_COUNT; ++i) {
        AtlasFrame frame = HUNTER_ATLAS_FRAMES[i];
        const int col = i % HUNTER_ATLAS_COLUMNS;
        const int row = i / HUNTER_ATLAS_COLUMNS;
        const int cellOriginX = col * (HUNTER_ATLAS_CELL_WIDTH + HUNTER_ATLAS_GUTTER);
        const int cellOriginY = row * (HUNTER_ATLAS_CELL_HEIGHT + HUNTER_ATLAS_GUTTER);

        frame.offsetX = frame.x - cellOriginX;
        frame.offsetY = frame.y - cellOriginY;
        frame.sourceWidth = HUNTER_ATLAS_CELL_WIDTH;
        frame.sourceHeight = HUNTER_ATLAS_CELL_HEIGHT;
        mData.frames.push_back(frame);
    }

    // ─── 4 حركات ──────────────────────────────────────────────────
    auto addClip = [&](const std::string& key,
                       const std::string& labelAr,
                       const std::string& labelEn,
                       const std::string& type,
                       const std::string& dir,
                       std::vector<int> frameIndices,
                       int fps, bool loop) {
        AnimationClip clip;
        clip.key = key;
        clip.labelAr = labelAr;
        clip.labelEn = labelEn;
        clip.type = type;
        clip.direction = dir;
        clip.frames = std::move(frameIndices);
        clip.fps = fps;
        clip.loop = loop;
        mData.animations.emplace(key, std::move(clip));
    };

    addClip("walk_left",  "مشي إلى اليسار", "Walk Left",  "walk", "left",  {0, 1, 2, 3, 4, 5, 6}, 8, true);
    addClip("walk_right", "مشي إلى اليمين", "Walk Right", "walk", "right", {0, 1, 2, 3, 4, 5, 6}, 8, true);
    addClip("idle_left",  "وقوف يسار",       "Idle Left",  "idle", "left",  {7}, 1, true);
    addClip("idle_right", "وقوف يمين",       "Idle Right", "idle", "right", {7}, 1, true);

    std::cout << "[Sprite] تم تحميل atlas الصياد الثابتة: " << HUNTER_ATLAS_FRAME_COUNT
              << " فريم (" << imgW << "x" << imgH << ")" << std::endl;
}

// ─── تحديث الحالة ────────────────────────────────────────────────

void SpriteAnimation::update(AnimationState& state, float deltaTime) const {
    auto it = mData.animations.find(state.currentClip);
    if (it == mData.animations.end()) return;

    const AnimationClip& clip = it->second;
    if (clip.frames.empty()) return;

    state.elapsed += deltaTime;

    int rawIndex = static_cast<int>(state.elapsed * static_cast<float>(clip.fps));

    if (clip.loop) {
        rawIndex = rawIndex % static_cast<int>(clip.frames.size());
        state.finished = false;
    } else {
        if (rawIndex >= static_cast<int>(clip.frames.size())) {
            rawIndex = static_cast<int>(clip.frames.size()) - 1;
            state.finished = true;
        }
    }

    state.currentFrameIndex = clip.frames[rawIndex];
}

// ─── بدء تشغيل مقطع ─────────────────────────────────────────────

void SpriteAnimation::play(AnimationState& state, const std::string& clipKey) const {
    if (state.currentClip == clipKey && !state.finished) return;

    state.currentClip = clipKey;
    state.elapsed = 0.0f;
    state.currentFrameIndex = 0;
    state.finished = false;

    auto it = mData.animations.find(clipKey);
    if (it != mData.animations.end() && !it->second.frames.empty()) {
        state.currentFrameIndex = it->second.frames[0];
        state.flipX = (it->second.direction == "left");
    }
}

// ─── حساب UV للفريم ─────────────────────────────────────────────

void SpriteAnimation::getFrameUV(int frameIndex, float& u0, float& u1, float& v0, float& v1) const {
    if (frameIndex < 0 || frameIndex >= static_cast<int>(mData.frames.size())) {
        u0 = u1 = v0 = v1 = 0.0f;
        return;
    }

    const AtlasFrame& frame = mData.frames[frameIndex];
    float imgW = static_cast<float>(mData.imageWidth);
    float imgH = static_cast<float>(mData.imageHeight);
    constexpr float texelInset = 0.5f;

    const float insetX = (frame.width > 1) ? texelInset : 0.0f;
    const float insetY = (frame.height > 1) ? texelInset : 0.0f;

    // سحب UV قليلًا للداخل لمنع sampler من قراءة بكسلات الفريم المجاور.
    u0 = (static_cast<float>(frame.x) + insetX) / imgW;
    u1 = (static_cast<float>(frame.x + frame.width) - insetX) / imgW;
    v0 = (static_cast<float>(frame.y) + insetY) / imgH;
    v1 = (static_cast<float>(frame.y + frame.height) - insetY) / imgH;
}

// ─── الحصول على مقطع ────────────────────────────────────────────

const AnimationClip* SpriteAnimation::getClip(const std::string& key) const {
    auto it = mData.animations.find(key);
    return (it != mData.animations.end()) ? &it->second : nullptr;
}

} // namespace game
