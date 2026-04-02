#include "HunterSpriteData.h"

#include <array>

namespace game {
namespace {

// فريمات الحركة الأساسية القادمة من sprite.png.
constexpr std::array<RawAtlasFrame, 8> HUNTER_MOVE_FRAMES = {{
    {19, 11, 235, 465},
    {311, 11, 253, 465},
    {614, 10, 249, 466},
    {923, 17, 232, 459},
    {23, 531, 227, 459},
    {322, 534, 231, 456},
    {622, 534, 233, 456},
    {923, 528, 233, 462},
}};

constexpr std::array<int, 7> HUNTER_WALK_FRAMES = {{0, 1, 2, 3, 4, 5, 6}};
constexpr std::array<int, 1> HUNTER_IDLE_FRAMES = {{7}};

// فريمات إطلاق النار القادمة من hunterGun.png وفق ترتيب الصورة الفعلي.
constexpr std::array<RawAtlasFrame, 7> HUNTER_SHOOT_FRAMES = {{
    {40, 56, 205, 322},
    {308, 61, 255, 299},
    {620, 66, 337, 299},
    {1047, 43, 385, 326},
    {48, 420, 322, 328},
    {489, 401, 222, 378},
    {752, 424, 197, 381},
}};

constexpr std::array<int, 5> HUNTER_SHOOT_SEQUENCE = {{1, 2, 3, 4, 5}};
constexpr std::array<int, 1> HUNTER_SHOOT_RECOVER_SEQUENCE = {{5}};
constexpr std::array<int, 1> HUNTER_SHOOT_READY_SEQUENCE = {{6}};

// جميع الأرقام الخاصة بأطلس الحركة الأساسية مجمعة هنا لتسهيل ضبطها لاحقًا.
const HunterMoveAtlasConfig HUNTER_MOVE_CONFIG = {
    .imageWidth = 1176,
    .imageHeight = 1000,
    .columns = 4,
    .cellWidth = 273,
    .cellHeight = 486,
    .gutter = 28,
    .frames = std::span<const RawAtlasFrame>(HUNTER_MOVE_FRAMES),
    .walkFrames = std::span<const int>(HUNTER_WALK_FRAMES),
    .idleFrames = std::span<const int>(HUNTER_IDLE_FRAMES),
};

// جميع الأرقام الخاصة بأطلس الإطلاق مجمعة هنا بدل نشرها داخل ملفات متعددة.
const HunterShootAtlasConfig HUNTER_SHOOT_CONFIG = {
    .imageWidth = 1442,
    .imageHeight = 823,
    .sourceWidth = 385,
    .sourceHeight = 381,
    .frames = std::span<const RawAtlasFrame>(HUNTER_SHOOT_FRAMES),
    .shootFrames = std::span<const int>(HUNTER_SHOOT_SEQUENCE),
    .shootRecoverFrames = std::span<const int>(HUNTER_SHOOT_RECOVER_SEQUENCE),
    .shootReadyFrames = std::span<const int>(HUNTER_SHOOT_READY_SEQUENCE),
};

// توقيتات السلاح تبقى في هذا الملف حتى يسهل ضبط الإحساس العام من مكان واحد.
const HunterActionTiming HUNTER_ACTION_TIMING = {
    .reloadDurationSeconds = 0.65f,
    .shootRecoverHoldSeconds = 3.0f,
    .shootReadySettleSeconds = 0.14f,
};

} // namespace

const HunterMoveAtlasConfig& hunterMoveAtlasConfig()
{
    return HUNTER_MOVE_CONFIG;
}

const HunterShootAtlasConfig& hunterShootAtlasConfig()
{
    return HUNTER_SHOOT_CONFIG;
}

const HunterActionTiming& hunterActionTiming()
{
    return HUNTER_ACTION_TIMING;
}

} // namespace game
