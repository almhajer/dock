#include "HunterSpriteAtlas.h"
#include "HunterSpriteData.h"

#include <span>
#include <utility>

namespace game {

namespace {

void addClip(SpriteAtlasData& data,
             const std::string& key,
             std::span<const int> frames,
             int fps,
             bool loop)
{
    AnimationClip clip;
    clip.key = key;
    clip.frames.assign(frames.begin(), frames.end());
    clip.fps = fps;
    clip.loop = loop;
    data.animations.emplace(key, std::move(clip));
}

void addDirectionalClipPair(SpriteAtlasData& data,
                            const std::string& baseName,
                            std::span<const int> frames,
                            int fps,
                            bool loop)
{
    addClip(data, baseName + "_left", frames, fps, loop);
    addClip(data, baseName + "_right", frames, fps, loop);
}

AtlasFrame makeMoveFrame(const RawAtlasFrame& raw, std::size_t index, const HunterMoveAtlasConfig& config)
{
    AtlasFrame frame;
    frame.x = raw.x;
    frame.y = raw.y;
    frame.width = raw.width;
    frame.height = raw.height;

    const int col = static_cast<int>(index) % config.columns;
    const int row = static_cast<int>(index) / config.columns;
    const int cellOriginX = col * (config.cellWidth + config.gutter);
    const int cellOriginY = row * (config.cellHeight + config.gutter);
    const int topOffsetY = frame.y - cellOriginY;

    frame.offsetX = frame.x - cellOriginX;
    frame.offsetY = config.cellHeight - topOffsetY - frame.height;
    frame.sourceWidth = config.cellWidth;
    frame.sourceHeight = config.cellHeight;
    return frame;
}

AtlasFrame makeShootFrame(const RawAtlasFrame& raw, const HunterShootAtlasConfig& config)
{
    AtlasFrame frame;
    frame.x = raw.x;
    frame.y = raw.y;
    frame.width = raw.width;
    frame.height = raw.height;
    frame.offsetX = (config.sourceWidth - raw.width) / 2;
    frame.offsetY = config.sourceHeight - raw.height;
    frame.sourceWidth = config.sourceWidth;
    frame.sourceHeight = config.sourceHeight;
    return frame;
}

} // namespace

SpriteAtlasData createHunterSpriteAtlasData(int imageWidth, int imageHeight)
{
    const HunterMoveAtlasConfig& config = hunterMoveAtlasConfig();

    SpriteAtlasData data;
    data.imageWidth = imageWidth;
    data.imageHeight = imageHeight;
    data.frames.reserve(config.frames.size());

    // نحول البيانات الخام القادمة من ملف الإعدادات إلى فريمات منطقية تستخدمها اللعبة.
    for (std::size_t index = 0; index < config.frames.size(); ++index)
    {
        data.frames.push_back(makeMoveFrame(config.frames[index], index, config));
    }

    addDirectionalClipPair(data, "walk", config.walkFrames, 8, true);
    addDirectionalClipPair(data, "idle", config.idleFrames, 1, true);

    if (imageWidth != config.imageWidth || imageHeight != config.imageHeight)
    {
        data.imageWidth = imageWidth;
        data.imageHeight = imageHeight;
    }

    return data;
}

SpriteAtlasData createHunterShootSpriteAtlasData(int imageWidth, int imageHeight)
{
    const HunterShootAtlasConfig& config = hunterShootAtlasConfig();

    SpriteAtlasData data;
    data.imageWidth = imageWidth;
    data.imageHeight = imageHeight;
    data.frames.reserve(config.frames.size());

    // أطلس الإطلاق مستقل حتى نتمكن من تعديل تسلسل فريماته من ملف بيانات واحد.
    for (const RawAtlasFrame& raw : config.frames)
    {
        data.frames.push_back(makeShootFrame(raw, config));
    }

    // الفريم السادس هو وضع التعافي بعد الإطلاق، والسابع وضع الجاهزية/الانتظار.
    addDirectionalClipPair(data, "shoot", config.shootFrames, 12, false);
    addDirectionalClipPair(data, "shoot_recover", config.shootRecoverFrames, 1, true);
    addDirectionalClipPair(data, "shoot_ready", config.shootReadyFrames, 1, true);

    if (imageWidth != config.imageWidth || imageHeight != config.imageHeight)
    {
        data.imageWidth = imageWidth;
        data.imageHeight = imageHeight;
    }

    return data;
}

} // namespace game
