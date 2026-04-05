#include "HunterSpriteAtlas.h"

#include <array>
#include <utility>

namespace game {
namespace {

// ─── بيانات الفريمات المستخرجة من hunter_vulkan_atlas.json ──────────
//
// التخطيط داخل hunter_atlas.png (3328×3264):
//   صف 0: walk_right  (8 فريمات)
//   صف 1: walk_left   (8 فريمات — لا نستخدمها، نعتمد على flipX)
//   صف 2: shoot_right (5 فريمات)
//   صف 3: shoot_up    (5 فريمات)
//
// خلية منطقية موحدة: 416×816 بكسل، المحتوى محاذاة من الأسفل عند y=792.
// x,y = موقع المحتوى داخل الصورة
// offsetX,offsetY = إزاحة داخل الخلية المنطقية (من أعلى اليسار)

constexpr int ATLAS_CELL_W = 416;
constexpr int ATLAS_CELL_H = 816;

struct FrameDef {
    int x, y, w, h;
    int offX, offY;
};

// فريمات المشي يمينًا (مؤشرات 0-7)
// تم توحيد offsetY=314 للفريمات الأطول من الوقوف (قص من الأعلى) لتثبيت الارتفاع.
constexpr std::array<FrameDef, 8> WALK_FRAMES = {{
    {86,   314, 243, 478, 86,  314},   // 0 (قُصّ 5 بكسل من الأعلى)
    {493,  314, 262, 478, 77,  314},   // 1 (قُصّ 3 بكسل من الأعلى)
    {910,  314, 259, 478, 78,  314},   // 2 (قُصّ 6 بكسل من الأعلى)
    {1336, 317, 240, 475, 88,  317},   // 3
    {1755, 315, 234, 477, 91,  315},   // 4
    {2169, 319, 238, 473, 89,  319},   // 5
    {2583, 319, 241, 473, 87,  319},   // 6
    {2999, 314, 242, 478, 87,  314},   // 7 (وقوف)
}};

// فريمات الإطلاق العادي (مؤشرات 8-12)
constexpr std::array<FrameDef, 5> SHOOT_FRAMES = {{
    {64,   1937, 288, 487, 64,  305},  // 8
    {487,  1979, 274, 445, 71,  347},  // 9
    {859,  1875, 362, 549, 27,  243},  // 10
    {1319, 1947, 274, 477, 71,  315},  // 11
    {1726, 1782, 292, 642, 62,  150},  // 12
}};

// فريمات الإطلاق العالي (مؤشرات 13-17)
constexpr std::array<FrameDef, 5> SHOOT_UP_FRAMES = {{
    {97,   2610, 221, 630, 97,  162},  // 13
    {511,  2563, 225, 677, 95,  115},  // 14
    {916,  2532, 248, 708, 84,  84},   // 15
    {1329, 2485, 253, 755, 81,  37},   // 16
    {1730, 2492, 284, 748, 66,  44},   // 17
}};

// ─── بناء AtlasFrame من FrameDef ────────────────────────────────

AtlasFrame makeFrame(const FrameDef& def)
{
    return AtlasFrame{
        .x = def.x,
        .y = def.y,
        .width = def.w,
        .height = def.h,
        .offsetX = def.offX,
        .offsetY = def.offY,
        .sourceWidth = ATLAS_CELL_W,
        .sourceHeight = ATLAS_CELL_H,
    };
}

// ─── إضافة مقطع حركة ───────────────────────────────────────────

void addClip(SpriteAtlasData& data,
             const std::string& key,
             std::initializer_list<int> frameIndices,
             int fps,
             bool loop)
{
    AnimationClip clip;
    clip.key = key;
    clip.frames.assign(frameIndices);
    clip.fps = fps;
    clip.loop = loop;
    data.animations.emplace(key, std::move(clip));
}

void addDirectionalPair(SpriteAtlasData& data,
                        const std::string& baseName,
                        std::initializer_list<int> frameIndices,
                        int fps,
                        bool loop)
{
    addClip(data, baseName + "_right", frameIndices, fps, loop);
    addClip(data, baseName + "_left",  frameIndices, fps, loop);
}

} // namespace

SpriteAtlasData createHunterAtlasData(int imageWidth, int imageHeight)
{
    SpriteAtlasData data;
    data.imageWidth = imageWidth;
    data.imageHeight = imageHeight;

    // 18 فريم: 8 مشي + 5 إطلاق عادي + 5 إطلاق عالي
    data.frames.reserve(WALK_FRAMES.size() + SHOOT_FRAMES.size() + SHOOT_UP_FRAMES.size());

    for (const auto& f : WALK_FRAMES)
        data.frames.push_back(makeFrame(f));

    for (const auto& f : SHOOT_FRAMES)
        data.frames.push_back(makeFrame(f));

    for (const auto& f : SHOOT_UP_FRAMES)
        data.frames.push_back(makeFrame(f));

    // ─── مقاطع الحركة ──────────────────────────────────────────
    // المشي والوقوف: نفس فريمات walk_right، flipX يعكس الاتجاه.
    addDirectionalPair(data, "walk", {0, 1, 2, 3, 4, 5, 6, 7}, 8, true);
    addDirectionalPair(data, "idle", {7}, 1, true);

    // إطلاق عادي: أمامي ثم عكسي متدرج للعودة.
    addDirectionalPair(data, "shoot",          {8, 9, 10, 11, 12},               14, false);
    addDirectionalPair(data, "shoot_recover",  {12, 11, 10, 9, 8},              10, false);

    // إطلاق عالي: أمامي ثم عكسي متدرج للعودة.
    addDirectionalPair(data, "shoot_up",         {13, 14, 15, 16, 17},           14, false);
    addDirectionalPair(data, "shoot_up_recover",  {17, 16, 15, 14, 13},         10, false);

    return data;
}

} // namespace game
