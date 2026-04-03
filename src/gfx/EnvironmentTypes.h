#pragma once

#include "RenderTypes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gfx {

inline constexpr std::size_t ENVIRONMENT_BASE_VERTEX_COUNT = 6;

enum class EnvironmentElementKind : uint32_t {
    Cloud = 0,
    Tree = 1,
};

enum class EnvironmentSpriteId : uint32_t {
    CloudWide = 0,
    CloudTower = 1,
    CloudWisp = 2,
    TreeRound = 3,
    TreeTall = 4,
    TreeLean = 5,
};

/// هندسة quad ثابتة تُستخدم مع instancing لرسم عناصر البيئة.
struct EnvironmentQuadVertex {
    float x = 0.0f;
    float y = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
};

/// بيانات instance واحدة لعناصر البيئة: غيمة أو شجرة.
struct EnvironmentInstance {
    float centerX = 0.0f;
    float centerY = 0.0f;
    float halfWidth = 0.0f;
    float halfHeight = 0.0f;

    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;

    float tintR = 1.0f;
    float tintG = 1.0f;
    float tintB = 1.0f;
    float alpha = 1.0f;

    float windWeight = 0.0f;
    float windPhase = 0.0f;
    float windResponse = 0.0f;
    float parallax = 1.0f;

    float kind = 0.0f;
    float softness = 0.0f;
    float swayAmount = 0.0f;
    float reserved = 0.0f;
};

/// دفعات البيئة مفصولة حسب النوع لتفادي خلط مسار الغيم مع مسار الشجر داخل نفس الـ shader.
struct EnvironmentRenderData {
    std::vector<EnvironmentInstance> cloudInstances;
    std::vector<EnvironmentInstance> backgroundTreeInstances;
    std::vector<EnvironmentInstance> foregroundTreeInstances;
};

} // namespace gfx
