#pragma once

#include "DuckPool.h"
#include "StageDefinitions.h"

#include <random>

namespace core
{

/*
 إحصائيات الدفعات داخل المرحلة
 */
struct WaveStats
{
    int totalDucks = 0;       // إجمالي البط في المرحلة
    int ducksSpawned = 0;     // بط ظهر حتى الآن
    int ducksHit = 0;         // بط أصيب
    int ducksDespawned = 0;   // بط اختفى (هرب أو سقط)
    int wavesSpawned = 0;     // عدد الدفعات التي ظهرت
};

/*
 مدير دفعات البط - يقرر متى وكم بطة تظهر
 */
class WaveManager
{
  public:
    /*
     يهيئ المدير لمرحلة جديدة
     */
    void startStage(const stage::StageConfig& config, float difficultyMultiplier);

    /*
     يُحدّث المدير كل إطار - يتحقق من وقت الدفعة التالية
     */
    void update(float deltaTime, DuckPool& pool, const DuckSpawnContext& spawnCtx);

    /*
     يُسجّل إصابة بطة
     */
    void onDuckHit();

    /*
     يُسجّل اختفاء بطة (هربت أو تلاشت)
     */
    void onDuckDespawned();

    /*
     هل انتهت المرحلة (كل البط ظهر واختفى)؟
     */
    [[nodiscard]] bool isStageComplete() const;

    /*
     هل يمكن بدء دفعة جديدة الآن؟
     */
    [[nodiscard]] bool canSpawnWave() const;

    /*
     الإحصائيات الحالية
     */
    [[nodiscard]] const WaveStats& stats() const
    {
        return mStats;
    }

    /*
     تقدم المرحلة كنسبة (0-1)
     */
    [[nodiscard]] float progress() const;

  private:
    WaveStats mStats;
    stage::StageConfig mConfig{};
    float mDifficultyMultiplier = 1.0f;
    float mWaveCooldown = 0.0f;
    bool mWaitingForDespawn = false;

    /*
     يحسب حجم الدفعة التالية عشوائياً
     */
    int computeWaveSize() const;

    /*
     يحسب الفاصل الزمني بين الدفعات
     */
    float computeWaveDelay() const;
};

} // namespace core
