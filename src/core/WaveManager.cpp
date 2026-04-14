#include "WaveManager.h"

#include <algorithm>
#include <cmath>

namespace core
{

void WaveManager::startStage(const stage::StageConfig& config, float difficultyMultiplier)
{
    mConfig = config;
    mDifficultyMultiplier = difficultyMultiplier;
    mStats = {};
    mStats.totalDucks = config.totalDucks;
    mWaveCooldown = 0.5f; // فاصل قصير قبل أول دفعة
    mIntraWaveCooldown = 0.0f;
    mWaitingForDespawn = false;
    mPendingWaveSize = 0;
    mPendingWaveSpawned = 0;
}

void WaveManager::update(float deltaTime, DuckPool& pool, const DuckSpawnContext& spawnCtx)
{
    /*
     * إذا كانت هناك موجة قيد الإطلاق بالفعل، نخرج بطاتها واحدةً بعد أخرى
     * بفاصل قصير. هذا يمنع خروج المجموعة كلها في نفس الإطار.
     */
    if (mPendingWaveSpawned < mPendingWaveSize)
    {
        if (mIntraWaveCooldown > 0.0f)
        {
            mIntraWaveCooldown -= deltaTime;
            return;
        }

        DuckSpawnContext duckCtx = spawnCtx;
        duckCtx.waveSize = mPendingWaveSize;
        duckCtx.duckIndex = mPendingWaveSpawned;
        const DuckId id = pool.spawn(duckCtx);
        if (id >= 0)
        {
            ++mPendingWaveSpawned;
            ++mStats.ducksSpawned;
            mIntraWaveCooldown = computeIntraWaveDelay();
        }
        return;
    }

    // إذا كانت هناك بطات نشطة، انتظر حتى تنتهي الموجة الحالية قبل بدء موجة جديدة
    if (pool.hasAnyActive())
    {
        mWaitingForDespawn = true;
        return;
    }

    // هل بقت بط لم تُظهر بعد؟
    if (mStats.ducksSpawned >= mStats.totalDucks)
    {
        return;
    }

    // انتظر الفاصل الزمني
    if (mWaveCooldown > 0.0f)
    {
        mWaveCooldown -= deltaTime;
        return;
    }

    // جهّز دفعة جديدة ليتم إطلاقها تدريجياً
    const int waveSize = computeWaveSize();
    const int remaining = mStats.totalDucks - mStats.ducksSpawned;
    const int toSpawn = std::min(waveSize, remaining);
    mPendingWaveSize = toSpawn;
    mPendingWaveSpawned = 0;
    mIntraWaveCooldown = 0.0f;
    ++mStats.wavesSpawned;
    mWaveCooldown = computeWaveDelay();
    mWaitingForDespawn = false;
}

void WaveManager::onDuckHit()
{
    ++mStats.ducksHit;
}

void WaveManager::onDuckDespawned()
{
    ++mStats.ducksDespawned;
}

bool WaveManager::isStageComplete() const
{
    return mStats.ducksSpawned >= mStats.totalDucks && mStats.ducksDespawned >= mStats.ducksSpawned;
}

bool WaveManager::canSpawnWave() const
{
    return mStats.ducksSpawned < mStats.totalDucks && mWaveCooldown <= 0.0f;
}

float WaveManager::progress() const
{
    if (mStats.totalDucks <= 0)
    {
        return 1.0f;
    }
    return static_cast<float>(mStats.ducksDespawned) / static_cast<float>(mStats.totalDucks);
}

int WaveManager::computeWaveSize() const
{
    const int remaining = mStats.totalDucks - mStats.ducksSpawned;

    // المرحلة الأولى: بطة واحدة دائماً
    if (mConfig.stageNumber <= 1)
    {
        return 1;
    }

    // المراحل 2: 1-2 بطات
    // المراحل 3-5: 1-3 بطات
    // المراحل 6+: 1-4 بطات (أو أكثر حسب الصعوبة)
    int maxWave = 1;
    if (mConfig.stageNumber >= 2)
    {
        maxWave = 2;
    }
    if (mConfig.stageNumber >= 3)
    {
        maxWave = 3;
    }
    if (mConfig.stageNumber >= 6)
    {
        maxWave = 4;
    }

    // التصعيد داخل المرحلة: كل دفعة لاحقة يمكن أن تكون أكبر
    const float stageProgress = static_cast<float>(mStats.ducksSpawned) / std::max(mStats.totalDucks, 1);
    const float escalationBonus = stageProgress * 0.5f * mDifficultyMultiplier;
    maxWave += static_cast<int>(escalationBonus);

    // لا تتجاوز المتبقي أو الحد الأقصى
    maxWave = std::min(maxWave, remaining);
    maxWave = std::min(maxWave, kMaxSimultaneousDucks);

    // عشوائي بين 1 و maxWave
    if (maxWave <= 1)
    {
        return 1;
    }

    // توزيع يميل نحو الأعداد الأصغر
    const int waveSize = 1 + (std::rand() % maxWave);
    return std::min(waveSize, remaining);
}

float WaveManager::computeWaveDelay() const
{
    // الفاصل الأساسي: 0.8-1.5 ثانية
    float baseDelay = 1.0f;

    // يقل مع تقدم المرحلة (تصعيد)
    const float progressRatio = static_cast<float>(mStats.ducksSpawned) / std::max(mStats.totalDucks, 1);
    baseDelay -= progressRatio * 0.3f;

    // يقل مع تقدم المراحل (مراحل أصعب)
    baseDelay -= static_cast<float>(mConfig.stageNumber - 1) * 0.03f;

    // يُعدّل حسب الصعوبة
    baseDelay /= mDifficultyMultiplier;

    // حدود عادلة
    return std::clamp(baseDelay, 0.4f, 2.0f);
}

float WaveManager::computeIntraWaveDelay() const
{
    /*
     * فاصل صغير بين بطات الدفعة نفسها حتى تبدو مواقع الخروج متفرقة
     * وحركتها أقل تكتلاً، مع تسريع تدريجي في المراحل المتقدمة.
     */
    float delay = 0.22f;
    delay -= static_cast<float>(std::max(mConfig.stageNumber - 1, 0)) * 0.01f;
    delay /= std::max(mDifficultyMultiplier, 0.6f);
    return std::clamp(delay, 0.08f, 0.25f);
}

} // namespace core
