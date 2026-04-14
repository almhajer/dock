#pragma once

#include "DuckGameplay.h"
#include "../game/SpriteAnimation.h"
#include "../gfx/RenderTypes.h"
#include "SceneLayout.h"

#include <random>
#include <array>

namespace core
{

/*
 مرحلة حياة البطة
 */
enum class DuckPhase : unsigned char
{
    Flying,   // تطير في السماء
    Falling,  // أُصيبت وتسقط
    Grounded, // وصلت الأرض وتنتظر الاختفاء
    Inactive, // غير نشطة
};

/*
 بيانات رحلة البطة العشوائية
 */
struct DuckFlightParams
{
    float startX = 0.0f;
    float endX = 0.0f;
    float startY = 0.0f;
    float endY = 0.0f;
    float controlY1 = 0.0f;
    float controlY2 = 0.0f;
    float arcHeight = 0.06f;
    float duration = 12.0f;
    float flapCycles = 2.8f;
    float swayPhase = 0.0f;
};

/*
 مثيل بطة واحدة مستقلة - تحمل كل حالتها بنفسها
 */
struct DuckInstance
{
    DuckPhase phase = DuckPhase::Inactive;

    // الموقع الحالي
    float x = 0.0f;
    float y = 0.0f;

    // رحلة الطيران
    float flightTime = 0.0f;
    DuckFlightParams flight;

    // فيزياء السقوط
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float rotation = 0.0f;
    float angularVelocity = 0.0f;

    // حالة الأرض
    float groundTimer = 0.0f;
    float alpha = 1.0f;

    // الاتجاه
    bool facingLeft = false;
    bool flipY = false;

    // حركة الرسم
    game::AnimationState animState;

    /*
     هل البطة نشطة (على الشاشة)؟
     */
    [[nodiscard]] bool isActive() const
    {
        return phase != DuckPhase::Inactive;
    }
};

/*
 معرّف بطة داخل المجمّع (-1 = غير صالح)
 */
using DuckId = int;

/*
 ثوابت المجمّع
 */
inline constexpr int kMaxSimultaneousDucks = 5;

/*
 سياق إنشاء بطة - يحدد حدود التوليد العشوائي
 */
struct DuckSpawnContext
{
    float flightDurationMin = 12.0f;
    float flightDurationMax = 14.0f;
    float arcMinHeight = 0.050f;
    float arcMaxHeight = 0.120f;
    float hunterX = 0.0f;
    scene::WindowMetrics metrics{};

    // معلومات الدفعة - لتوزيع البطات على مواقع مختلفة
    int waveSize = 1;     // عدد البط في هذه الدفعة
    int duckIndex = 0;    // فهرس هذه البطة داخل الدفعة (0-based)
};

/*
 تجمّع بطات متعددة - يدير دورة حياة حتى 5 بطات في آنٍ واحد
 */
class DuckPool
{
  public:
    /*
     تهيئة المجمّع
     */
    void initialize(game::SpriteAnimation* anim, std::mt19937* rng);

    /*
     يُنشئ بطة جديدة بمسار عشوائي ويعيد معرّفها
     */
    DuckId spawn(const DuckSpawnContext& ctx);

    /*
     يحرر بطة من المجمّع (تبدو غير نشطة)
     */
    void release(DuckId id);

    /*
     يُحدّث كل البط النشط
     */
    void update(float deltaTime);

    /*
     يحاول إصابة بطة عند الموقع المعطى ويعيد معرّفها أو -1
     */
    DuckId tryHit(float cursorX, float cursorY, const scene::WindowMetrics& metrics);

    /*
     يحرّر كل البط
     */
    void clearAll();

    /*
     يبني quads الرسم لكل البط النشطة
     */
    void buildRenderQuads(std::vector<gfx::TexturedQuad>& quads,
                          const scene::WindowMetrics& metrics) const;

    /*
     عدد البط النشطة حالياً
     */
    [[nodiscard]] int activeCount() const;

    /*
     هل أي بطة لا تزال تطير؟
     */
    [[nodiscard]] bool hasAnyFlying() const;

    /*
     هل يوجد بطة نشطة على الإطلاق؟
     */
    [[nodiscard]] bool hasAnyActive() const;

    /*
     عدد البط التي أصيبت منذ آخر clearAll()
     */
    [[nodiscard]] int totalHitCount() const
    {
        return mTotalHit;
    }

    /*
     عدد البط التي اختفت (هربت أو اختفت أرضياً) منذ آخر clearAll()
     */
    [[nodiscard]] int totalDespawnedCount() const
    {
        return mTotalDespawned;
    }

    /*
     إحصائيات إعادة الضبط
     */
    void resetStats()
    {
        mTotalHit = 0;
        mTotalDespawned = 0;
    }

    /*
     معاينة بطة بالمعرّف
     */
    [[nodiscard]] const DuckInstance& get(DuckId id) const
    {
        return mDucks[id];
    }

  private:
    std::array<DuckInstance, kMaxSimultaneousDucks> mDucks{};
    game::SpriteAnimation* mAnim = nullptr;
    std::mt19937* mRng = nullptr;
    int mTotalHit = 0;
    int mTotalDespawned = 0;

    void updateFlyingDuck(DuckInstance& duck, float deltaTime);
    void updateFallingDuck(DuckInstance& duck, float deltaTime);
    void updateGroundedDuck(DuckInstance& duck, float deltaTime);
    void randomizeFlight(DuckInstance& duck, const DuckSpawnContext& ctx);
};

} // namespace core
