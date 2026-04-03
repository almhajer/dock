#include "EnvironmentAtlas.h"

#include <array>
#include <algorithm>
#include <cmath>

namespace gfx {
namespace {

constexpr int ATLAS_CELL_SIZE = 256;
constexpr int ATLAS_COLUMNS = 3;
constexpr int ATLAS_ROWS = 2;
constexpr int ATLAS_WIDTH = ATLAS_COLUMNS * ATLAS_CELL_SIZE;
constexpr int ATLAS_HEIGHT = ATLAS_ROWS * ATLAS_CELL_SIZE;

struct SpriteAtlasEntry {
    EnvironmentSpriteId spriteId;
    EnvironmentElementKind kind;
    int x;
    int y;
    int width;
    int height;
};

constexpr std::array<SpriteAtlasEntry, 6> ATLAS_ENTRIES = {{
    {EnvironmentSpriteId::CloudWide, EnvironmentElementKind::Cloud, 0 * ATLAS_CELL_SIZE, 0, ATLAS_CELL_SIZE, ATLAS_CELL_SIZE},
    {EnvironmentSpriteId::CloudTower, EnvironmentElementKind::Cloud, 1 * ATLAS_CELL_SIZE, 0, ATLAS_CELL_SIZE, ATLAS_CELL_SIZE},
    {EnvironmentSpriteId::CloudWisp, EnvironmentElementKind::Cloud, 2 * ATLAS_CELL_SIZE, 0, ATLAS_CELL_SIZE, ATLAS_CELL_SIZE},
    {EnvironmentSpriteId::TreeRound, EnvironmentElementKind::Tree, 0 * ATLAS_CELL_SIZE, 1 * ATLAS_CELL_SIZE, ATLAS_CELL_SIZE, ATLAS_CELL_SIZE},
    {EnvironmentSpriteId::TreeTall, EnvironmentElementKind::Tree, 1 * ATLAS_CELL_SIZE, 1 * ATLAS_CELL_SIZE, ATLAS_CELL_SIZE, ATLAS_CELL_SIZE},
    {EnvironmentSpriteId::TreeLean, EnvironmentElementKind::Tree, 2 * ATLAS_CELL_SIZE, 1 * ATLAS_CELL_SIZE, ATLAS_CELL_SIZE, ATLAS_CELL_SIZE},
}};

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

[[nodiscard]] Vec2 operator+(const Vec2& lhs, const Vec2& rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

[[nodiscard]] Vec2 operator-(const Vec2& lhs, const Vec2& rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

[[nodiscard]] Vec2 operator*(const Vec2& value, float scale)
{
    return {value.x * scale, value.y * scale};
}

[[nodiscard]] float saturate(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

[[nodiscard]] float mixValue(float a, float b, float t)
{
    return a + (b - a) * saturate(t);
}

[[nodiscard]] Color mixColor(const Color& a, const Color& b, float t)
{
    return {
        mixValue(a.r, b.r, t),
        mixValue(a.g, b.g, t),
        mixValue(a.b, b.b, t),
    };
}

[[nodiscard]] float dot2(const Vec2& lhs, const Vec2& rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

float hash01(float value)
{
    const float raw = std::sin(value * 127.1f + 311.7f) * 43758.5453f;
    return raw - std::floor(raw);
}

float smooth01(float value)
{
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped * clamped * (3.0f - 2.0f * clamped);
}

float ellipseMask(float dx, float dy, float radiusX, float radiusY, float feather)
{
    const float nx = dx / std::max(radiusX, 0.0001f);
    const float ny = dy / std::max(radiusY, 0.0001f);
    const float distance = std::sqrt(nx * nx + ny * ny);
    return 1.0f - smooth01((distance - 1.0f + feather) / std::max(feather, 0.0001f));
}

float hash21(const Vec2& value, float seed)
{
    return hash01(value.x * 127.1f + value.y * 311.7f + seed * 74.7f);
}

float valueNoise(const Vec2& point, float seed)
{
    const Vec2 cell = {std::floor(point.x), std::floor(point.y)};
    const Vec2 local = {point.x - cell.x, point.y - cell.y};
    const Vec2 smooth = {
        local.x * local.x * (3.0f - 2.0f * local.x),
        local.y * local.y * (3.0f - 2.0f * local.y),
    };

    const float v00 = hash21(cell, seed);
    const float v10 = hash21({cell.x + 1.0f, cell.y}, seed);
    const float v01 = hash21({cell.x, cell.y + 1.0f}, seed);
    const float v11 = hash21({cell.x + 1.0f, cell.y + 1.0f}, seed);

    const float nx0 = mixValue(v00, v10, smooth.x);
    const float nx1 = mixValue(v01, v11, smooth.x);
    return mixValue(nx0, nx1, smooth.y);
}

float fbm(Vec2 point, float seed)
{
    float amplitude = 0.56f;
    float sum = 0.0f;
    float weight = 0.0f;

    for (int octave = 0; octave < 4; ++octave)
    {
        sum += valueNoise(point, seed + static_cast<float>(octave) * 17.3f) * amplitude;
        weight += amplitude;
        point = {point.x * 2.03f + 19.7f, point.y * 1.97f - 13.1f};
        amplitude *= 0.52f;
    }

    return sum / std::max(weight, 0.0001f);
}

float lineMask(const Vec2& from, const Vec2& to, const Vec2& point, float thickness, float feather)
{
    const Vec2 segment = to - from;
    const float denominator = std::max(dot2(segment, segment), 0.0001f);
    const float projection = std::clamp(dot2(point - from, segment) / denominator, 0.0f, 1.0f);
    const Vec2 closestPoint = from + segment * projection;
    const Vec2 delta = point - closestPoint;
    const float distance = std::sqrt(dot2(delta, delta));
    return 1.0f - smooth01((distance - thickness) / std::max(feather, 0.0001f));
}

void blendPixel(std::vector<unsigned char>& pixels,
                int atlasWidth,
                int x,
                int y,
                const Color& color,
                float alpha)
{
    if (x < 0 || y < 0)
    {
        return;
    }

    const int atlasHeight = static_cast<int>(pixels.size() / 4 / atlasWidth);
    if (x >= atlasWidth || y >= atlasHeight || alpha <= 0.0f)
    {
        return;
    }

    const std::size_t index =
        (static_cast<std::size_t>(y) * static_cast<std::size_t>(atlasWidth) + static_cast<std::size_t>(x)) * 4u;

    const float dstAlpha = static_cast<float>(pixels[index + 3]) / 255.0f;
    const float outAlpha = alpha + dstAlpha * (1.0f - alpha);
    if (outAlpha <= 0.0001f)
    {
        return;
    }

    const auto dstColor = [&](std::size_t component)
    {
        return static_cast<float>(pixels[index + component]) / 255.0f;
    };

    const float outR = (color.r * alpha + dstColor(0) * dstAlpha * (1.0f - alpha)) / outAlpha;
    const float outG = (color.g * alpha + dstColor(1) * dstAlpha * (1.0f - alpha)) / outAlpha;
    const float outB = (color.b * alpha + dstColor(2) * dstAlpha * (1.0f - alpha)) / outAlpha;

    pixels[index + 0] = static_cast<unsigned char>(std::clamp(outR, 0.0f, 1.0f) * 255.0f);
    pixels[index + 1] = static_cast<unsigned char>(std::clamp(outG, 0.0f, 1.0f) * 255.0f);
    pixels[index + 2] = static_cast<unsigned char>(std::clamp(outB, 0.0f, 1.0f) * 255.0f);
    pixels[index + 3] = static_cast<unsigned char>(std::clamp(outAlpha, 0.0f, 1.0f) * 255.0f);
}

const SpriteAtlasEntry& atlasEntry(EnvironmentSpriteId spriteId)
{
    for (const SpriteAtlasEntry& entry : ATLAS_ENTRIES)
    {
        if (entry.spriteId == spriteId)
        {
            return entry;
        }
    }

    return ATLAS_ENTRIES.front();
}

void drawCloudCell(std::vector<unsigned char>& pixels, const SpriteAtlasEntry& entry, float seed)
{
    const Color lightTop = {0.99f, 0.99f, 0.985f};
    const Color lightMid = {0.93f, 0.95f, 0.98f};
    const Color shadowBottom = {0.64f, 0.71f, 0.80f};
    const Color shadowCore = {0.71f, 0.77f, 0.85f};

    const float crownBias = 0.92f + hash01(seed + 0.7f) * 0.30f;
    const float widthBias = 0.98f + hash01(seed + 2.4f) * 0.18f;
    const float roundness = 0.94f + hash01(seed + 4.1f) * 0.10f;
    const int inset = 8;

    for (int py = entry.y + inset; py < entry.y + entry.height - inset; ++py)
    {
        for (int px = entry.x + inset; px < entry.x + entry.width - inset; ++px)
        {
            const float localX = static_cast<float>(px - entry.x) / static_cast<float>(entry.width);
            const float localY = static_cast<float>(py - entry.y) / static_cast<float>(entry.height);
            const Vec2 uv = {localX, localY};
            float coverage = 0.0f;
            float density = 0.0f;

            // الجسم الأساسي للغيمة بيضوي ومغلق حتى لا تظهر أي حواف مقطوعة.
            const float baseBody = ellipseMask(localX - 0.50f, localY - 0.48f, 0.36f * widthBias, 0.23f * roundness, 0.24f);
            const float coreBody = ellipseMask(localX - 0.50f, localY - 0.47f, 0.30f * widthBias, 0.19f * roundness, 0.28f);
            coverage = baseBody;
            density = coreBody * 0.52f + baseBody * 0.18f;

            // فصوص علوية ناعمة تضيف شكل السحب الركامية مع بقاء المحيط العام بيضويًا.
            for (int blobIndex = 0; blobIndex < 3; ++blobIndex)
            {
                const float blobSeed = seed * 13.0f + static_cast<float>(blobIndex) * 17.9f;
                const float arc = static_cast<float>(blobIndex) / 2.0f;
                const float crownLift = std::sin(arc * 3.1415926f) * 0.10f * crownBias;
                const float centerX = 0.30f + arc * 0.40f + (hash01(blobSeed + 1.2f) - 0.5f) * 0.05f;
                const float centerY = 0.33f - crownLift - hash01(blobSeed + 2.6f) * 0.03f;
                const float radiusX = (0.13f + hash01(blobSeed + 4.3f) * 0.05f) * widthBias;
                const float radiusY = (0.11f + hash01(blobSeed + 6.1f) * 0.04f) * roundness;
                const float blob = ellipseMask(localX - centerX, localY - centerY, radiusX, radiusY, 0.28f);
                coverage = std::max(coverage, blob);
                density += blob * (0.70f + hash01(blobSeed + 7.4f) * 0.28f);
            }

            // فصوص جانبية خفيفة للحفاظ على الإحساس البيضوي بدل الشكل المستطيل أو المقطوع.
            for (int sideIndex = 0; sideIndex < 2; ++sideIndex)
            {
                const float sideSeed = seed * 23.0f + static_cast<float>(sideIndex) * 11.7f;
                const float direction = (sideIndex == 0) ? -1.0f : 1.0f;
                const float centerX = 0.50f + direction * (0.22f + hash01(sideSeed + 1.0f) * 0.03f);
                const float centerY = 0.49f + (hash01(sideSeed + 2.0f) - 0.5f) * 0.03f;
                const float radiusX = 0.12f + hash01(sideSeed + 3.0f) * 0.04f;
                const float radiusY = 0.10f + hash01(sideSeed + 4.0f) * 0.04f;
                const float lobe = ellipseMask(localX - centerX, localY - centerY, radiusX, radiusY, 0.30f);
                coverage = std::max(coverage, lobe);
                density += lobe * 0.24f;
            }

            // غلاف بيضوي عام يمنع شكل القص ويجعل الأطراف ناعمة ومستمرة في كل الاتجاهات.
            const float outerEnvelope = ellipseMask(localX - 0.50f, localY - 0.47f, 0.41f * widthBias, 0.28f * roundness, 0.22f);
            const float innerEnvelope = ellipseMask(localX - 0.50f, localY - 0.47f, 0.34f * widthBias, 0.23f * roundness, 0.26f);
            coverage = std::max(coverage, baseBody * 0.92f);
            coverage = saturate(coverage * (0.68f + outerEnvelope * 0.32f));
            density += innerEnvelope * 0.22f;

            const float edgeNoise = fbm({uv.x * 7.8f + seed * 0.4f, uv.y * 8.2f - seed * 0.3f}, seed + 9.0f);
            const float erosion = 0.04f + (0.5f - edgeNoise) * 0.03f;
            coverage = saturate((coverage * (0.92f + outerEnvelope * 0.08f) - erosion) / std::max(1.0f - erosion, 0.0001f));
            density = saturate((density * 0.24f + coverage * 0.94f) * (0.96f + edgeNoise * 0.04f));
            if (coverage <= 0.001f)
            {
                continue;
            }

            const float topLight = smooth01((0.82f - localY) / 0.72f);
            const float coreLight = smooth01((density - 0.16f) / 0.60f);
            const float underside = smooth01((localY - 0.60f) / 0.20f);
            const float silverLining = smooth01((0.22f - localY) / 0.16f) * smooth01((0.88f - abs(localX - 0.5f) * 1.6f) / 0.88f);
            Color color = mixColor(shadowBottom, lightMid, topLight * 0.72f + coreLight * 0.18f);
            color = mixColor(color, shadowCore, underside * 0.58f);
            color = mixColor(color, lightTop, silverLining * 0.24f);

            const float alpha = coverage * (0.78f + density * 0.16f) * (0.96f + edgeNoise * 0.02f);
            blendPixel(pixels, ATLAS_WIDTH, px, py, color, alpha);
        }
    }
}

void drawTreeCell(std::vector<unsigned char>& pixels,
                  const SpriteAtlasEntry& entry,
                  float seed,
                  float canopyStretch,
                  float trunkLean)
{
    const Color trunkDark = {0.20f, 0.14f, 0.07f};
    const Color trunkLight = {0.38f, 0.28f, 0.16f};
    const Color leafShadow = {0.08f, 0.16f, 0.07f};
    const Color leafMid = {0.17f, 0.31f, 0.12f};
    const Color leafLight = {0.30f, 0.47f, 0.18f};

    const float trunkCenterX = 0.50f + trunkLean * 0.10f;
    const float trunkBaseWidth = 0.044f + hash01(seed + 4.9f) * 0.018f;
    const float trunkTopY = 0.46f + hash01(seed + 8.1f) * 0.06f;
    const Vec2 root = {trunkCenterX, 0.96f};
    const Vec2 mid = {trunkCenterX + trunkLean * 0.05f, 0.70f};
    const Vec2 crownBase = {trunkCenterX + trunkLean * 0.12f, trunkTopY};

    for (int py = entry.y + 10; py < entry.y + entry.height - 4; ++py)
    {
        for (int px = entry.x + 8; px < entry.x + entry.width - 8; ++px)
        {
            const float localX = static_cast<float>(px - entry.x) / static_cast<float>(entry.width);
            const float localY = static_cast<float>(py - entry.y) / static_cast<float>(entry.height);
            const Vec2 uv = {localX, localY};

            // الجذع يتدرج في السماكة مع الجذر ويُضاف له تفرع خفيف حتى لا تبدو الشجرة كتلة واحدة.
            const float trunkLerp = smooth01((localY - crownBase.y) / std::max(root.y - crownBase.y, 0.0001f));
            const float trunkWidth = mixValue(trunkBaseWidth * 0.48f, trunkBaseWidth, trunkLerp);
            float trunkMask = 0.0f;
            trunkMask = std::max(trunkMask, lineMask(root, mid, uv, trunkWidth, trunkWidth * 0.85f));
            trunkMask = std::max(trunkMask, lineMask(mid, crownBase, uv, trunkWidth * 0.74f, trunkWidth * 0.80f));
            trunkMask = std::max(trunkMask, ellipseMask(localX - root.x, localY - root.y, trunkBaseWidth * 1.55f, 0.06f, 0.22f));

            float branchMask = 0.0f;
            for (int branchIndex = 0; branchIndex < 5; ++branchIndex)
            {
                const float branchSeed = seed * 29.0f + static_cast<float>(branchIndex) * 13.7f;
                const float branchMix = 0.14f + static_cast<float>(branchIndex) * 0.15f;
                const Vec2 start = {
                    mixValue(mid.x, crownBase.x, branchMix),
                    mixValue(mid.y, crownBase.y, branchMix),
                };
                const float direction = ((branchIndex % 2 == 0) ? -1.0f : 1.0f) * (0.84f + hash01(branchSeed + 1.0f) * 0.44f);
                const Vec2 end = {
                    start.x + direction * (0.13f + hash01(branchSeed + 2.0f) * 0.14f) + trunkLean * 0.04f,
                    start.y - (0.05f + hash01(branchSeed + 3.0f) * 0.10f),
                };
                const float thickness = trunkBaseWidth * (0.18f + hash01(branchSeed + 4.0f) * 0.07f);
                branchMask = std::max(branchMask, lineMask(start, end, uv, thickness, thickness * 0.92f));
            }

            if (std::max(trunkMask, branchMask) > 0.01f)
            {
                const float barkNoise = fbm({localX * 14.0f + seed, localY * 20.0f - seed * 0.5f}, seed + 21.0f);
                const float barkLight = smooth01((0.60f - std::abs(localX - trunkCenterX) * 6.0f) / 0.60f);
                Color barkColor = mixColor(trunkDark, trunkLight, barkLight * 0.52f + barkNoise * 0.16f);
                barkColor = mixColor(barkColor, trunkDark, localY * 0.16f);
                blendPixel(pixels, ATLAS_WIDTH, px, py, barkColor, std::max(trunkMask, branchMask * 0.82f) * 0.96f);
            }

            float canopyCoverage = 0.0f;
            float canopyDensity = 0.0f;

            // قلب التاج يبقى موجودًا، لكنه أخف من السابق حتى تظهر بنية الأغصان والأوراق.
            for (int blobIndex = 0; blobIndex < 4; ++blobIndex)
            {
                const float blobSeed = seed * 13.0f + static_cast<float>(blobIndex) * 17.3f;
                const float span = static_cast<float>(blobIndex) / 3.0f;
                const float centerX = crownBase.x + (span - 0.5f) * (0.22f + canopyStretch * 0.08f) +
                                      (hash01(blobSeed + 1.7f) - 0.5f) * 0.05f;
                const float centerY = 0.30f - std::sin(span * 3.1415926f) * 0.05f -
                                      hash01(blobSeed + 4.4f) * 0.05f;
                const float radiusX = (0.10f + hash01(blobSeed + 7.2f) * 0.08f) * (0.86f + canopyStretch * 0.18f);
                const float radiusY = 0.09f + hash01(blobSeed + 9.8f) * 0.08f;
                const float blob = ellipseMask(localX - centerX, localY - centerY, radiusX, radiusY, 0.25f);
                canopyCoverage = std::max(canopyCoverage, blob);
                canopyDensity += blob * (0.48f + hash01(blobSeed + 10.9f) * 0.24f);
            }

            // الأوراق تتوزع على نهايات الأغصان وعلى امتدادها حتى تبدو الشجرة أقرب وأكثر طبيعية.
            for (int branchIndex = 0; branchIndex < 5; ++branchIndex)
            {
                const float branchSeed = seed * 41.0f + static_cast<float>(branchIndex) * 19.1f;
                const float branchMix = 0.14f + static_cast<float>(branchIndex) * 0.15f;
                const Vec2 start = {
                    mixValue(mid.x, crownBase.x, branchMix),
                    mixValue(mid.y, crownBase.y, branchMix),
                };
                const float direction = ((branchIndex % 2 == 0) ? -1.0f : 1.0f) * (0.84f + hash01(branchSeed + 1.0f) * 0.44f);
                const Vec2 end = {
                    start.x + direction * (0.13f + hash01(branchSeed + 2.0f) * 0.14f) + trunkLean * 0.04f,
                    start.y - (0.05f + hash01(branchSeed + 3.0f) * 0.10f),
                };

                const Vec2 outerLeaf = {
                    mixValue(start.x, end.x, 0.82f),
                    mixValue(start.y, end.y, 0.82f),
                };
                const Vec2 midLeaf = {
                    mixValue(start.x, end.x, 0.58f),
                    mixValue(start.y, end.y, 0.58f),
                };
                const Vec2 tipLeaf = {
                    end.x + direction * (0.02f + hash01(branchSeed + 5.0f) * 0.03f),
                    end.y - (0.01f + hash01(branchSeed + 6.0f) * 0.03f),
                };

                const float outerLeafMask = ellipseMask(
                    localX - outerLeaf.x,
                    localY - outerLeaf.y,
                    (0.07f + hash01(branchSeed + 7.0f) * 0.06f) * (0.88f + canopyStretch * 0.18f),
                    0.07f + hash01(branchSeed + 8.0f) * 0.05f,
                    0.28f);
                const float midLeafMask = ellipseMask(
                    localX - midLeaf.x,
                    localY - midLeaf.y,
                    (0.06f + hash01(branchSeed + 9.0f) * 0.05f) * (0.86f + canopyStretch * 0.16f),
                    0.06f + hash01(branchSeed + 10.0f) * 0.05f,
                    0.30f);
                const float tipLeafMask = ellipseMask(
                    localX - tipLeaf.x,
                    localY - tipLeaf.y,
                    (0.08f + hash01(branchSeed + 11.0f) * 0.06f) * (0.92f + canopyStretch * 0.20f),
                    0.08f + hash01(branchSeed + 12.0f) * 0.06f,
                    0.28f);

                canopyCoverage = std::max(canopyCoverage, outerLeafMask);
                canopyCoverage = std::max(canopyCoverage, midLeafMask);
                canopyCoverage = std::max(canopyCoverage, tipLeafMask);
                canopyDensity += outerLeafMask * 0.34f + midLeafMask * 0.26f + tipLeafMask * 0.46f;
            }

            const float canopyNoise = fbm({localX * 8.4f + seed * 0.6f, localY * 8.9f - seed * 0.4f}, seed + 31.0f);
            const float edgeCut = 0.14f + (0.5f - canopyNoise) * 0.10f;
            canopyCoverage = saturate((canopyCoverage - edgeCut) / std::max(1.0f - edgeCut, 0.0001f));
            canopyDensity = saturate((canopyDensity * 0.42f + canopyCoverage * 0.84f) * (0.92f + canopyNoise * 0.12f));
            const float holeNoise = valueNoise({localX * 12.0f + seed * 0.3f, localY * 11.0f + seed * 0.7f}, seed + 47.0f);
            const float innerHole = smooth01((holeNoise - 0.88f) / 0.10f) * smooth01((canopyDensity - 0.46f) / 0.24f);
            canopyCoverage *= 1.0f - innerHole * 0.30f;
            if (canopyCoverage <= 0.01f)
            {
                continue;
            }

            const float topLight = smooth01((0.84f - localY) / 0.82f);
            const float sideLight = smooth01((0.72f - std::abs(localX - crownBase.x) * 2.4f) / 0.72f);
            const float innerShade = smooth01((localY - 0.28f) / 0.42f);
            const float leafMix = saturate(topLight * 0.42f + sideLight * 0.34f + canopyNoise * 0.16f);
            Color canopyColor = mixColor(leafShadow, leafMid, leafMix);
            canopyColor = mixColor(canopyColor, leafLight, topLight * 0.38f + sideLight * 0.14f);
            canopyColor = mixColor(canopyColor, leafShadow, innerShade * 0.18f);

            const float canopyAlpha = canopyCoverage * (0.88f + canopyDensity * 0.10f + canopyNoise * 0.06f);
            blendPixel(pixels, ATLAS_WIDTH, px, py, canopyColor, canopyAlpha);
        }
    }
}

} // namespace

EnvironmentAtlasBitmap createEnvironmentAtlasBitmap()
{
    EnvironmentAtlasBitmap atlas;
    atlas.width = ATLAS_WIDTH;
    atlas.height = ATLAS_HEIGHT;
    atlas.pixels.assign(static_cast<std::size_t>(atlas.width) * static_cast<std::size_t>(atlas.height) * 4u, 0u);

    drawCloudCell(atlas.pixels, atlasEntry(EnvironmentSpriteId::CloudWide), 1.0f);
    drawCloudCell(atlas.pixels, atlasEntry(EnvironmentSpriteId::CloudTower), 3.0f);
    drawCloudCell(atlas.pixels, atlasEntry(EnvironmentSpriteId::CloudWisp), 5.0f);

    drawTreeCell(atlas.pixels, atlasEntry(EnvironmentSpriteId::TreeRound), 7.0f, 1.0f, 0.0f);
    drawTreeCell(atlas.pixels, atlasEntry(EnvironmentSpriteId::TreeTall), 9.0f, 0.82f, -0.12f);
    drawTreeCell(atlas.pixels, atlasEntry(EnvironmentSpriteId::TreeLean), 11.0f, 1.12f, 0.18f);

    return atlas;
}

UvRect environmentAtlasUv(EnvironmentSpriteId spriteId)
{
    const SpriteAtlasEntry& entry = atlasEntry(spriteId);
    return {
        static_cast<float>(entry.x) / static_cast<float>(ATLAS_WIDTH),
        static_cast<float>(entry.x + entry.width) / static_cast<float>(ATLAS_WIDTH),
        static_cast<float>(entry.y) / static_cast<float>(ATLAS_HEIGHT),
        static_cast<float>(entry.y + entry.height) / static_cast<float>(ATLAS_HEIGHT),
    };
}

EnvironmentElementKind environmentSpriteKind(EnvironmentSpriteId spriteId)
{
    return atlasEntry(spriteId).kind;
}

} // namespace gfx
