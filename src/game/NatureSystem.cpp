#include "NatureSystem.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace game {
namespace {

constexpr float CLOUD_WRAP_PADDING = 0.28f;
constexpr float TREE_ROOT_SINK = 0.012f;

float cloudAltitude(float bandHash, float offsetHash)
{
    constexpr std::array<float, 4> LEVELS = {-0.94f, -0.82f, -0.68f, -0.54f};
    const std::size_t levelIndex = std::min(
        static_cast<std::size_t>(bandHash * static_cast<float>(LEVELS.size())),
        LEVELS.size() - 1);
    return LEVELS[levelIndex] + (offsetHash - 0.5f) * 0.07f;
}

gfx::EnvironmentSpriteId cloudSpriteByIndex(int index)
{
    constexpr std::array<gfx::EnvironmentSpriteId, 3> SPRITES = {
        gfx::EnvironmentSpriteId::CloudWide,
        gfx::EnvironmentSpriteId::CloudTower,
        gfx::EnvironmentSpriteId::CloudWisp,
    };
    return SPRITES[static_cast<std::size_t>(index) % SPRITES.size()];
}

gfx::EnvironmentSpriteId treeSpriteByIndex(int index)
{
    constexpr std::array<gfx::EnvironmentSpriteId, 3> SPRITES = {
        gfx::EnvironmentSpriteId::TreeRound,
        gfx::EnvironmentSpriteId::TreeTall,
        gfx::EnvironmentSpriteId::TreeLean,
    };
    return SPRITES[static_cast<std::size_t>(index) % SPRITES.size()];
}

void sortByDepth(std::vector<gfx::EnvironmentInstance>& instances)
{
    if (instances.size() < 2)
    {
        return;
    }

    std::sort(instances.begin(), instances.end(), [](const gfx::EnvironmentInstance& lhs, const gfx::EnvironmentInstance& rhs)
    {
        if (lhs.parallax == rhs.parallax)
        {
            return lhs.centerY < rhs.centerY;
        }
        return lhs.parallax < rhs.parallax;
    });
}

} // namespace

float NatureSystem::hash01(float value)
{
    const float raw = std::sin(value * 127.1f + 311.7f) * 43758.5453f;
    return raw - std::floor(raw);
}

void NatureSystem::initialize(const core::scene::WindowMetrics& metrics)
{
    if (!metrics.valid())
    {
        return;
    }

    rebuildLayout(metrics);
    mInitialized = true;
}

void NatureSystem::update(float deltaTime, const core::scene::WindowMetrics& metrics)
{
    if (!metrics.valid())
    {
        return;
    }

    if (!mInitialized || metrics.width != mLayoutWidth || metrics.height != mLayoutHeight)
    {
        rebuildLayout(metrics);
        mInitialized = true;
    }

    mWind.time += deltaTime;
    mWind.gustStrength = 0.6f + 0.4f * std::sin(mWind.time * 0.17f);
    updateClouds(deltaTime, metrics);
}

void NatureSystem::buildRenderData(const core::scene::WindowMetrics& metrics,
                                   gfx::EnvironmentRenderData& renderData) const
{
    const std::size_t cloudCapacity = mClouds.size();
    const std::size_t backgroundTreeCapacity = mBackgroundTreeRenderCache.size();
    const std::size_t foregroundTreeCapacity = mForegroundTreeRenderCache.size();
    renderData.cloudInstances.clear();
    renderData.backgroundTreeInstances.clear();
    renderData.foregroundTreeInstances.clear();

    // نعيد استخدام الذاكرة بين الإطارات بدل إنشاء دفعات جديدة كل مرة.
    if (renderData.cloudInstances.capacity() < cloudCapacity)
    {
        renderData.cloudInstances.reserve(cloudCapacity);
    }
    if (renderData.backgroundTreeInstances.capacity() < backgroundTreeCapacity)
    {
        renderData.backgroundTreeInstances.reserve(backgroundTreeCapacity);
    }
    if (renderData.foregroundTreeInstances.capacity() < foregroundTreeCapacity)
    {
        renderData.foregroundTreeInstances.reserve(foregroundTreeCapacity);
    }

    for (const CloudState& cloud : mClouds)
    {
        const gfx::UvRect uv = gfx::environmentAtlasUv(cloud.sprite);
        const float tint = cloud.tint;
        const float windResponse = 0.24f + cloud.parallax * 0.12f;
        renderData.cloudInstances.push_back({
            cloud.x,
            cloud.y,
            cloud.width * 0.5f,
            cloud.height * 0.5f,
            uv.u0,
            uv.v0,
            uv.u1,
            uv.v1,
            tint,
            tint,
            tint * 1.06f,
            cloud.alpha,
            0.56f,
            cloud.verticalPhase,
            windResponse,
            cloud.parallax,
            static_cast<float>(gfx::EnvironmentElementKind::Cloud == gfx::environmentSpriteKind(cloud.sprite) ? 0 : 1),
            0.84f,
            0.24f,
            0.0f,
        });
    }

    if (!mBackgroundTreeRenderCache.empty())
    {
        renderData.backgroundTreeInstances.insert(
            renderData.backgroundTreeInstances.end(),
            mBackgroundTreeRenderCache.begin(),
            mBackgroundTreeRenderCache.end());
    }
    if (!mForegroundTreeRenderCache.empty())
    {
        renderData.foregroundTreeInstances.insert(
            renderData.foregroundTreeInstances.end(),
            mForegroundTreeRenderCache.begin(),
            mForegroundTreeRenderCache.end());
    }

    sortByDepth(renderData.cloudInstances);
    sortByDepth(renderData.backgroundTreeInstances);
    sortByDepth(renderData.foregroundTreeInstances);
    (void)metrics;
}

void NatureSystem::rebuildLayout(const core::scene::WindowMetrics& metrics)
{
    mLayoutWidth = metrics.width;
    mLayoutHeight = metrics.height;
    mClouds.clear();
    mTrees.clear();
    mRecycleCursor = 0.0f;
    mWind.time = 0.0f;
    mWind.direction = 1.0f;
    mWind.baseSpeed = 0.010f;
    mWind.driftStrength = 0.028f;
    mWind.gustStrength = 0.0f;

    const int cloudCount = std::clamp(static_cast<int>(5 + metrics.width / 420), 7, 11);
    mClouds.reserve(static_cast<std::size_t>(cloudCount));

    std::vector<float> cloudWeights(static_cast<std::size_t>(cloudCount), 1.0f);
    float cloudWeightSum = 0.0f;
    for (int index = 0; index < cloudCount; ++index)
    {
        const float seed = static_cast<float>(index) * 3.17f + 1.0f;
        cloudWeights[static_cast<std::size_t>(index)] = 0.52f + hash01(seed + 21.0f) * 1.35f;
        cloudWeightSum += cloudWeights[static_cast<std::size_t>(index)];
    }

    float accumulatedCloudWeight = 0.0f;

    for (int index = 0; index < cloudCount; ++index)
    {
        const float seed = static_cast<float>(index) * 3.17f + 1.0f;
        accumulatedCloudWeight += cloudWeights[static_cast<std::size_t>(index)];
        const float progress = accumulatedCloudWeight / std::max(cloudWeightSum, 0.0001f);
        CloudState cloud;
        cloud.sprite = cloudSpriteByIndex(index);
        cloud.parallax = 0.68f + hash01(seed + 2.0f) * 0.42f;
        cloud.width = (0.38f + hash01(seed + 4.0f) * 0.34f) / metrics.aspect;
        cloud.height = cloud.width * (0.40f + hash01(seed + 7.0f) * 0.12f) * metrics.aspect;
        cloud.x = -1.24f + progress * 2.48f + (hash01(seed + 6.0f) - 0.5f) * 0.18f;
        // السحب تتوزع على طبقات ارتفاع مختلفة بدل الالتصاق بأعلى الشاشة.
        cloud.y = cloudAltitude(hash01(seed + 8.0f), hash01(seed + 8.7f));
        cloud.speed = (0.010f + hash01(seed + 9.0f) * 0.014f) * cloud.parallax;
        cloud.heading = 0.90f + (hash01(seed + 10.7f) - 0.5f) * 0.16f;
        cloud.verticalPhase = hash01(seed + 11.0f) * 6.2831853f;
        cloud.verticalDrift = 0.006f + hash01(seed + 13.0f) * 0.010f;
        cloud.alpha = 0.60f + hash01(seed + 15.0f) * 0.16f;
        cloud.tint = 0.96f + hash01(seed + 17.0f) * 0.05f;
        mClouds.push_back(cloud);
    }

    const int backgroundTreeCount = std::clamp(static_cast<int>(5 + metrics.width / 480), 7, 10);
    const int foregroundTreeCount = 4;
    mTrees.reserve(static_cast<std::size_t>(backgroundTreeCount + foregroundTreeCount));

    std::vector<float> backgroundWeights(static_cast<std::size_t>(backgroundTreeCount), 1.0f);
    float backgroundWeightSum = 0.0f;
    for (int index = 0; index < backgroundTreeCount; ++index)
    {
        const float seed = static_cast<float>(index) * 4.11f + 31.0f;
        backgroundWeights[static_cast<std::size_t>(index)] = 0.58f + hash01(seed + 18.0f) * 1.08f;
        backgroundWeightSum += backgroundWeights[static_cast<std::size_t>(index)];
    }

    float accumulatedWeight = 0.0f;

    for (int index = 0; index < backgroundTreeCount; ++index)
    {
        const float seed = static_cast<float>(index) * 4.11f + 31.0f;
        accumulatedWeight += backgroundWeights[static_cast<std::size_t>(index)];
        const float progress = accumulatedWeight / std::max(backgroundWeightSum, 0.0001f);
        TreeState tree;
        tree.foreground = false;
        tree.sprite = treeSpriteByIndex(index);
        tree.parallax = 0.62f + hash01(seed + 1.0f) * 0.22f;
        tree.height = 0.50f + hash01(seed + 2.0f) * 0.16f;
        tree.width = tree.height * (0.30f + hash01(seed + 4.0f) * 0.10f) / metrics.aspect;
        tree.x = -1.16f + progress * 2.32f + (hash01(seed + 7.0f) - 0.5f) * 0.10f;
        if (std::abs(tree.x) < 0.26f)
        {
            tree.x += (tree.x < 0.0f ? -0.26f : 0.26f);
        }
        tree.baseY = core::scene::groundSurfaceY() + (hash01(seed + 7.8f) - 0.5f) * TREE_ROOT_SINK * 0.55f;
        tree.phase = hash01(seed + 9.0f) * 6.2831853f;
        tree.swayAmount = 0.032f + hash01(seed + 12.0f) * 0.016f;
        tree.alpha = 0.62f + hash01(seed + 13.0f) * 0.14f;
        tree.tint = 0.84f + hash01(seed + 15.0f) * 0.10f;
        mTrees.push_back(tree);
    }

    constexpr std::array<float, foregroundTreeCount> FOREGROUND_X = {-1.06f, -0.70f, 0.68f, 1.02f};
    for (int index = 0; index < foregroundTreeCount; ++index)
    {
        const float seed = static_cast<float>(index) * 5.37f + 71.0f;
        TreeState tree;
        tree.foreground = true;
        tree.sprite = treeSpriteByIndex(index + 1);
        tree.parallax = 1.10f + hash01(seed + 1.0f) * 0.18f;
        tree.height = 1.16f + hash01(seed + 3.0f) * 0.24f;
        tree.width = tree.height * (0.34f + hash01(seed + 5.0f) * 0.08f) / metrics.aspect;
        tree.x = FOREGROUND_X[static_cast<std::size_t>(index)] + (hash01(seed + 7.0f) - 0.5f) * 0.05f;
        tree.baseY = core::scene::groundSurfaceY() + 0.034f + hash01(seed + 8.3f) * 0.008f;
        tree.phase = hash01(seed + 9.0f) * 6.2831853f;
        tree.swayAmount = 0.052f + hash01(seed + 11.0f) * 0.018f;
        tree.alpha = 0.88f + hash01(seed + 13.0f) * 0.10f;
        tree.tint = 0.94f + hash01(seed + 15.0f) * 0.08f;
        mTrees.push_back(tree);
    }

    // الأشجار ثابتة مكانيًا بعد بناء التخطيط، لذلك نحوّلها مرة واحدة إلى instancing data.
    rebuildTreeRenderCaches();
}

void NatureSystem::rebuildTreeRenderCaches()
{
    mBackgroundTreeRenderCache.clear();
    mForegroundTreeRenderCache.clear();
    mBackgroundTreeRenderCache.reserve(mTrees.size());
    mForegroundTreeRenderCache.reserve(mTrees.size());

    for (const TreeState& tree : mTrees)
    {
        const gfx::UvRect uv = gfx::environmentAtlasUv(tree.sprite);
        const float tintR = tree.tint * 0.98f;
        const float tintG = tree.tint;
        const float tintB = tree.tint * 0.92f;
        gfx::EnvironmentInstance instance = {
            tree.x,
            tree.baseY - tree.height * 0.5f,
            tree.width * 0.5f,
            tree.height * 0.5f,
            uv.u0,
            uv.v0,
            uv.u1,
            uv.v1,
            tintR,
            tintG,
            tintB,
            tree.alpha,
            tree.foreground ? 1.0f : 0.88f,
            tree.phase,
            0.86f + tree.parallax * 0.14f,
            tree.parallax,
            1.0f,
            tree.foreground ? 0.34f : 0.22f,
            tree.swayAmount,
            0.0f,
        };

        if (tree.foreground)
        {
            mForegroundTreeRenderCache.push_back(instance);
        }
        else
        {
            mBackgroundTreeRenderCache.push_back(instance);
        }
    }

    sortByDepth(mBackgroundTreeRenderCache);
    sortByDepth(mForegroundTreeRenderCache);
}

void NatureSystem::updateClouds(float deltaTime, const core::scene::WindowMetrics& metrics)
{
    for (CloudState& cloud : mClouds)
    {
        const float gust = 0.88f + mWind.gustStrength * 0.18f;
        const float travelWave =
            std::sin(mWind.time * (0.08f + cloud.parallax * 0.02f) + cloud.verticalPhase) * 0.12f;
        const float randomDrift =
            std::cos(mWind.time * 0.05f + cloud.verticalPhase * 0.73f) * mWind.driftStrength * 0.26f;
        cloud.x += (
            mWind.direction * mWind.baseSpeed +
            cloud.speed * cloud.heading * (0.94f + travelWave) +
            randomDrift) *
            gust * deltaTime;
        cloud.y += (
            std::sin(mWind.time * (0.07f + cloud.parallax * 0.02f) + cloud.verticalPhase) *
            cloud.verticalDrift * 0.16f +
            std::cos(mWind.time * 0.05f + cloud.verticalPhase * 0.6f) * mWind.driftStrength * 0.04f) *
            deltaTime;

        const float wrapWidth = cloud.width * 0.5f + CLOUD_WRAP_PADDING;
        if (cloud.x - wrapWidth > 1.28f)
        {
            recycleCloud(cloud, metrics, true, mRecycleCursor);
            mRecycleCursor += 1.0f;
        }
        else if (cloud.x + wrapWidth < -1.28f)
        {
            recycleCloud(cloud, metrics, false, mRecycleCursor);
            mRecycleCursor += 1.0f;
        }
    }
}

void NatureSystem::recycleCloud(CloudState& cloud,
                                const core::scene::WindowMetrics& metrics,
                                bool wrapToLeft,
                                float seed) const
{
    const float value = seed * 7.17f + cloud.verticalPhase;
    cloud.sprite = cloudSpriteByIndex(static_cast<int>(seed) + static_cast<int>(cloud.sprite));
    cloud.parallax = 0.68f + hash01(value + 1.0f) * 0.42f;
    cloud.width = (0.38f + hash01(value + 2.0f) * 0.34f) / metrics.aspect;
    cloud.height = cloud.width * (0.40f + hash01(value + 3.0f) * 0.12f) * metrics.aspect;
    cloud.x = wrapToLeft ? -1.38f - cloud.width : 1.38f + cloud.width;
    cloud.y = cloudAltitude(hash01(value + 4.0f), hash01(value + 4.7f));
    cloud.speed = (0.010f + hash01(value + 5.0f) * 0.014f) * cloud.parallax;
    cloud.heading = 0.90f + (hash01(value + 5.9f) - 0.5f) * 0.16f;
    cloud.verticalPhase = hash01(value + 6.0f) * 6.2831853f;
    cloud.verticalDrift = 0.006f + hash01(value + 7.0f) * 0.010f;
    cloud.alpha = 0.60f + hash01(value + 8.0f) * 0.16f;
    cloud.tint = 0.96f + hash01(value + 9.0f) * 0.05f;
}

} // namespace game
