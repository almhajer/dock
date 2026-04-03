#pragma once

#include "../core/SceneLayout.h"
#include "../gfx/EnvironmentAtlas.h"
#include "../gfx/EnvironmentTypes.h"

#include <cstdint>
#include <vector>

namespace game {

struct WindField {
    float time = 0.0f;
    float direction = 1.0f;
    float baseSpeed = 0.12f;
    float driftStrength = 0.04f;
    float gustStrength = 0.0f;
};

struct CloudState {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float speed = 0.0f;
    float heading = 1.0f;
    float verticalPhase = 0.0f;
    float verticalDrift = 0.0f;
    float parallax = 1.0f;
    float alpha = 1.0f;
    float tint = 1.0f;
    gfx::EnvironmentSpriteId sprite = gfx::EnvironmentSpriteId::CloudWide;
};

struct TreeState {
    float x = 0.0f;
    float baseY = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float phase = 0.0f;
    float swayAmount = 0.0f;
    float parallax = 1.0f;
    float alpha = 1.0f;
    float tint = 1.0f;
    bool foreground = false;
    gfx::EnvironmentSpriteId sprite = gfx::EnvironmentSpriteId::TreeRound;
};

/// نظام البيئة المسؤول عن تحديث الغيوم والأشجار وبناء دفعات الرسم فقط.
class NatureSystem {
public:
    void initialize(const core::scene::WindowMetrics& metrics);
    void update(float deltaTime, const core::scene::WindowMetrics& metrics);
    void buildRenderData(const core::scene::WindowMetrics& metrics,
                         gfx::EnvironmentRenderData& renderData) const;

private:
    void rebuildLayout(const core::scene::WindowMetrics& metrics);
    void rebuildTreeRenderCaches();
    void updateClouds(float deltaTime, const core::scene::WindowMetrics& metrics);
    void recycleCloud(CloudState& cloud, const core::scene::WindowMetrics& metrics, bool wrapToLeft, float seed) const;

    [[nodiscard]] static float hash01(float value);

    bool mInitialized = false;
    uint32_t mLayoutWidth = 0;
    uint32_t mLayoutHeight = 0;
    float mRecycleCursor = 0.0f;
    WindField mWind;
    std::vector<CloudState> mClouds;
    std::vector<TreeState> mTrees;
    std::vector<gfx::EnvironmentInstance> mBackgroundTreeRenderCache;
    std::vector<gfx::EnvironmentInstance> mForegroundTreeRenderCache;
};

} // namespace game
