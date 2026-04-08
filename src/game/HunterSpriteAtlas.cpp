#include "HunterSpriteAtlas.h"

#include <array>
#include <utility>

namespace game {
namespace {

// ─── ثوابت اللوحة المرجعية للشخصية ──────────────────────────────
//
// sourceSize (450×500): الحجم المنطقي المرجعي الموحّد لجميع الفريمات.
// يمثل "اللوحة الوهمية" التي تُوضع عليها الشخصية قبل القص.
//
// pivot (0.5, 0.9): نقطة الارتكاز — منتصف الأسفل (تحت القدمين).
// تُستخدم لربط الشخصية بنقطة ثابتة على الشاشة (خط الأرض).
//
// FEET_Y: الموضع العمودي لأسفل القدمين داخل اللوحة المرجعية.
// جميع الفريمات تُحاذى بحيث تكون أقدام الشخصية عند هذا المستوى.
//
// حساب spriteX و spriteY تلقائيًا:
//   spriteX = (SOURCE_W - frameW) / 2   ← توسيط أفقي
//   spriteY = FEET_Y - frameH           ← محاذاة الأسفل مع مستوى القدمين

constexpr int SOURCE_W = 450;
constexpr int SOURCE_H = 500;
constexpr float PIVOT_X = 0.5f;
constexpr float PIVOT_Y = 0.9f;
constexpr int FEET_Y = 488; // = SOURCE_H - 12

// ─── تعريف فريم داخل الأطلس ────────────────────────────────────
// الإحداثيات (x, y, w, h) تم استخراجها من الحدود الفعلية غير الشفافة
// داخل hunter_atlas.png الحالي (3328×3264).
// spriteX و spriteY تُحسب تلقائيًا من الثوابت أعلاه.

struct FrameDef {
    int x, y, w, h;
    int durationMs;
};

// ─── فريمات المشي الأساسية (الصف الأول فقط، متجهة لليمين) ───────────
// الصف الثاني في الأطلس هو نسخة معكوسة من هذه الفريمات، لذلك لا نضمه
// إلى دورة المشي حتى لا تختلط الجهات مع نظام flipX الحالي.

constexpr std::array<FrameDef, 8> WALK_FRAMES = {{
    {86,   309, 243, 483, 80},    // walk_00
    {493,  311, 262, 481, 80},    // walk_01
    {910,  308, 259, 484, 80},    // walk_02
    {1336, 317, 240, 475, 80},    // walk_03
    {1755, 315, 234, 477, 80},    // walk_04
    {2169, 319, 238, 473, 80},    // walk_05
    {2583, 319, 241, 473, 80},    // walk_06
    {2999, 314, 242, 478, 80},    // walk_07
}};

// ─── فريمات الإطلاق للأمام (5 فريمات) ────────────────────────────

constexpr std::array<FrameDef, 5> SHOOT_FORWARD_FRAMES = {{
    {64,   1937, 288, 487, 70},   // shoot_forward_00
    {487,  1979, 274, 445, 70},   // shoot_forward_01
    {859,  1859, 376, 565, 70},   // shoot_forward_02
    {1319, 1857, 450, 567, 70},   // shoot_forward_03
    {1827, 1768, 304, 656, 90},   // shoot_forward_04
}};

// ─── فريمات الإطلاق للأعلى (5 فريمات) ────────────────────────────

constexpr std::array<FrameDef, 5> SHOOT_UP_FRAMES = {{
    {97,   2610, 221, 630, 70},   // shoot_up_00
    {511,  2543, 241, 697, 70},   // shoot_up_01
    {916,  2514, 266, 726, 70},   // shoot_up_02
    {1329, 2465, 267, 775, 70},   // shoot_up_03
    {1730, 2483, 297, 757, 90},   // shoot_up_04
}};

// ─── بناء AtlasFrame من FrameDef ────────────────────────────────

AtlasFrame makeFrame(const FrameDef& def)
{
    return AtlasFrame{
        .x = def.x,
        .y = def.y,
        .width = def.w,
        .height = def.h,
        .sourceW = SOURCE_W,
        .sourceH = SOURCE_H,
        .spriteX = (SOURCE_W - def.w) / 2,
        .spriteY = FEET_Y - def.h,
        .pivotX = PIVOT_X,
        .pivotY = PIVOT_Y,
        .durationMs = def.durationMs,
    };
}

// ─── إدارة المقاطع ──────────────────────────────────────────────

void addClip(SpriteAtlasData& data,
             const std::string& key,
             std::initializer_list<int> frameIndices,
             bool loop)
{
    AnimationClip clip;
    clip.key = key;
    clip.frames.assign(frameIndices);
    clip.loop = loop;

    for (int frameIndex : clip.frames)
    {
        clip.totalDurationMs += data.frames[frameIndex].durationMs;
    }

    data.animations.emplace(key, std::move(clip));
}

void addDirectionalPair(SpriteAtlasData& data,
                        const std::string& baseName,
                        std::initializer_list<int> frameIndices,
                        bool loop)
{
    addClip(data, baseName + "_right", frameIndices, loop);
    addClip(data, baseName + "_left",  frameIndices, loop);
}

} // namespace

SpriteAtlasData createHunterAtlasData(int imageWidth, int imageHeight)
{
    SpriteAtlasData data;
    data.imageWidth = imageWidth;
    data.imageHeight = imageHeight;

    // 18 فريم: 8 مشي + 5 إطلاق أمامي + 5 إطلاق عالي
    data.frames.reserve(
        WALK_FRAMES.size() + SHOOT_FORWARD_FRAMES.size() + SHOOT_UP_FRAMES.size());

    for (const auto& f : WALK_FRAMES)
        data.frames.push_back(makeFrame(f));

    for (const auto& f : SHOOT_FORWARD_FRAMES)
        data.frames.push_back(makeFrame(f));

    for (const auto& f : SHOOT_UP_FRAMES)
        data.frames.push_back(makeFrame(f));

    // ─── مقاطع الحركة ──────────────────────────────────────────
    addDirectionalPair(data, "walk",
        {0, 1, 2, 3, 4, 5, 6, 7}, true);

    addDirectionalPair(data, "idle", {7}, true);

    addDirectionalPair(data, "shoot",
        {8, 9, 10, 11, 12}, false);

    addDirectionalPair(data, "shoot_up",
        {13, 14, 15, 16, 17}, false);

    return data;
}

} // namespace game
