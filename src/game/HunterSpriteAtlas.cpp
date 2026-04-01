#include "HunterSpriteAtlas.h"

#include <array>
#include <span>
#include <utility>

namespace game {

namespace {

constexpr int HUNTER_ATLAS_IMAGE_WIDTH = 1176;
constexpr int HUNTER_ATLAS_IMAGE_HEIGHT = 1000;
constexpr int HUNTER_ATLAS_COLUMNS = 4;
constexpr int HUNTER_ATLAS_CELL_WIDTH = 273;
constexpr int HUNTER_ATLAS_CELL_HEIGHT = 486;
constexpr int HUNTER_ATLAS_GUTTER = 28;

constexpr std::array<AtlasFrame, 8> HUNTER_ATLAS_FRAMES = {{
    {19, 11, 235, 465},
    {311, 11, 253, 465},
    {614, 10, 249, 466},
    {923, 17, 232, 459},
    {23, 531, 227, 459},
    {322, 534, 231, 456},
    {622, 534, 233, 456},
    {923, 528, 233, 462},
}};

constexpr std::array<int, 7> WALK_FRAMES = {{0, 1, 2, 3, 4, 5, 6}};
constexpr std::array<int, 1> IDLE_FRAMES = {{7}};

void addClip(SpriteAtlasData& data,
             const std::string& key,
             std::span<const int> frames,
             int fps,
             bool loop) {
    AnimationClip clip;
    clip.key = key;
    clip.frames.assign(frames.begin(), frames.end());
    clip.fps = fps;
    clip.loop = loop;
    data.animations.emplace(key, std::move(clip));
}

template <std::size_t FrameCount>
void addDirectionalClipPair(SpriteAtlasData& data,
                            const std::string& baseName,
                            const std::array<int, FrameCount>& frames,
                            int fps,
                            bool loop) {
    addClip(data, baseName + "_left", std::span<const int>(frames), fps, loop);
    addClip(data, baseName + "_right", std::span<const int>(frames), fps, loop);
}

} // namespace

SpriteAtlasData createHunterSpriteAtlasData(int imageWidth, int imageHeight) {
    SpriteAtlasData data;
    data.imageWidth = imageWidth;
    data.imageHeight = imageHeight;
    data.frames.reserve(HUNTER_ATLAS_FRAMES.size());

    for (size_t i = 0; i < HUNTER_ATLAS_FRAMES.size(); ++i) {
        AtlasFrame frame = HUNTER_ATLAS_FRAMES[i];
        const int col = static_cast<int>(i) % HUNTER_ATLAS_COLUMNS;
        const int row = static_cast<int>(i) / HUNTER_ATLAS_COLUMNS;
        const int cellOriginX = col * (HUNTER_ATLAS_CELL_WIDTH + HUNTER_ATLAS_GUTTER);
        const int cellOriginY = row * (HUNTER_ATLAS_CELL_HEIGHT + HUNTER_ATLAS_GUTTER);
        const int topOffsetY = frame.y - cellOriginY;

        frame.offsetX = frame.x - cellOriginX;
        frame.offsetY = HUNTER_ATLAS_CELL_HEIGHT - topOffsetY - frame.height;
        frame.sourceWidth = HUNTER_ATLAS_CELL_WIDTH;
        frame.sourceHeight = HUNTER_ATLAS_CELL_HEIGHT;
        data.frames.push_back(frame);
    }

    addDirectionalClipPair(data, "walk", WALK_FRAMES, 8, true);
    addDirectionalClipPair(data, "idle", IDLE_FRAMES, 1, true);

    // الإبقاء على الأبعاد المتوقعة ضمن هذا الملف يجعل atlas قابلة للاستبدال لاحقًا
    // بملف مشابه من دون لمس كود الأنيميشن نفسه.
    if (imageWidth == HUNTER_ATLAS_IMAGE_WIDTH && imageHeight == HUNTER_ATLAS_IMAGE_HEIGHT) {
        return data;
    }

    return data;
}

} // namespace game
