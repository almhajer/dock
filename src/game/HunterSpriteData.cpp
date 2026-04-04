#include "HunterSpriteData.h"

#include <array>

namespace game {
namespace {

constexpr RawAtlasFrame makePackedFrame(int x,
                                        int y,
                                        int width,
                                        int height,
                                        int sourceWidth = 0,
                                        int sourceHeight = 0,
                                        int sourceX = 0,
                                        int sourceY = 0)
{
    return RawAtlasFrame{
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .sourceX = sourceX,
        .sourceY = sourceY,
        .sourceWidth = sourceWidth,
        .sourceHeight = sourceHeight,
    };
}

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

// هذه الفريمات مُحددة من الصورة نفسها بعد فتح hunterGun.png والتحقق البصري من مواضعها الفعلية.
// الصورة Packed، لذلك نثبت كل الفريمات داخل نفس الصندوق المنطقي ونحدد الإزاحات يدويًا
// بدل التمركز الآلي الذي كان يجعل الجسم يقفز للخلف مع الفريمات العريضة.
constexpr std::array<RawAtlasFrame, 7> HUNTER_SHOOT_FRAMES = {{
    makePackedFrame(40, 56, 205, 322, 385, 381, 110, 59),
    makePackedFrame(308, 61, 305, 299, 385, 381, 96, 82),
    // هذا الفريم يحتاج عرضًا أكبر حتى يحتوي كامل امتداد مؤثر الإطلاق داخل الإطار المنطقي.
    makePackedFrame(620, 66, 385, 299, 385, 381, 109, 82),
    makePackedFrame(1047, 43, 385, 326, 385, 381, 111, 55),
    makePackedFrame(48, 420, 322, 328, 385, 381, 111, 53),
    makePackedFrame(489, 401, 222, 378, 385, 381, 81, 3),
    makePackedFrame(752, 424, 197, 381, 385, 381, 93, 0),
}};

// تسلسل ما بعد الإطلاق المطلوب:
// 1) تمر الحركة عبر الفريم الخامس بدون تثبيت.
// 2) الوقفة القصيرة تكون على الفريم السادس.
// 3) إذا لم توجد طلقة جديدة ينتقل الصياد إلى الفريم السابع.
constexpr std::array<int, 5> HUNTER_SHOOT_SEQUENCE = {{1, 2, 3, 4, 5}};
constexpr std::array<int, 0> HUNTER_SHOOT_HOLD_SEQUENCE = {};
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
    // الفريمات هنا تحمل source sizes صريحة مع offsets مخصصة، لذلك لا نستخدم المسار الافتراضي.
    .defaultSourceWidth = 0,
    .defaultSourceHeight = 0,
    .frames = std::span<const RawAtlasFrame>(HUNTER_SHOOT_FRAMES),
    .shootFrames = std::span<const int>(HUNTER_SHOOT_SEQUENCE),
    .shootHoldFrames = std::span<const int>(HUNTER_SHOOT_HOLD_SEQUENCE),
    .shootRecoverFrames = std::span<const int>(HUNTER_SHOOT_RECOVER_SEQUENCE),
    .shootReadyFrames = std::span<const int>(HUNTER_SHOOT_READY_SEQUENCE),
};

// توقيتات السلاح تبقى في هذا الملف حتى يسهل ضبط الإحساس العام من مكان واحد.
const HunterActionTiming HUNTER_ACTION_TIMING = {
    .reloadDurationSeconds = 0.65f,
    .shootRecoverHoldSeconds = 3.0f,
    .shootReadySettleSeconds = 0.0f,
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
