/**
 * @file DuckSpriteAtlas.cpp
 * @brief تنفيذ أطلس البطة.
 * @details بناء وإدارة أطلس البطة من الصورة المصدر.
 */

#include "DuckSpriteAtlas.h"

#include "../gfx/stb_image.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace game
{
namespace
{

constexpr int FRAME_COUNT = 11;
constexpr int FRAME_COLUMNS = 4;
constexpr int OUTPUT_SAFE_WIDTH = 224;
constexpr int OUTPUT_SAFE_HEIGHT = 224;
constexpr int COMPONENT_MIN_AREA = 6;
constexpr int ANCHOR_MIN_AREA = 6000;
constexpr int ANCHOR_MERGE_MARGIN_X = 72;
constexpr int ANCHOR_MERGE_MARGIN_Y = 72;
constexpr int CELL_PADDING = 2;
constexpr float PIVOT_X = 0.5f;
constexpr float PIVOT_Y = 0.5f;

struct FrameDef
{
    int durationMs = 80;
};

struct PixelCoord
{
    int x = 0;
    int y = 0;
};

struct Component
{
    int minX = std::numeric_limits<int>::max();
    int minY = std::numeric_limits<int>::max();
    int maxX = 0; // exclusive
    int maxY = 0; // exclusive
    int area = 0;
    float centerX = 0.0f;
    float centerY = 0.0f;
    std::vector<PixelCoord> pixels;
};

struct RawFrame
{
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
};

struct FrameSize
{
    int width = 0;
    int height = 0;
};

struct ReferenceFrame
{
    int canvasWidth = 0;
    int canvasHeight = 0;
    int bboxX = 0;
    int bboxY = 0;
    int bboxWidth = 0;
    int bboxHeight = 0;
};

struct PreparedFrame
{
    int width = 0;
    int height = 0;
    int sourceW = 0;
    int sourceH = 0;
    int spriteX = 0;
    int spriteY = 0;
    std::vector<unsigned char> pixels;
};

[[nodiscard]] std::vector<ReferenceFrame> makeFallbackReferenceFrames(const std::vector<RawFrame>& sourceFrames);

constexpr std::array<FrameDef, FRAME_COUNT> FRAME_DEFS = {{
    {140},
    {140},
    {140},
    {140},
    {140},
    {140},
    {160},
    {160},
    {160},
    {160},
    {160},
}};

[[nodiscard]] std::size_t pixelOffset(int width, int x, int y)
{
    return (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
}

[[nodiscard]] unsigned char alphaAt(const unsigned char* pixels, int width, int x, int y)
{
    return pixels[pixelOffset(width, x, y) + 3u];
}

[[nodiscard]] Component finalizeComponent(Component component)
{
    component.area = static_cast<int>(component.pixels.size());
    component.centerX = static_cast<float>(component.minX + component.maxX) * 0.5f;
    component.centerY = static_cast<float>(component.minY + component.maxY) * 0.5f;
    return component;
}

[[nodiscard]] std::vector<Component> extractComponents(const unsigned char* sourcePixels, int sourceWidth,
                                                       int sourceHeight)
{
    std::vector<Component> components;
    std::vector<std::uint8_t> visited(static_cast<std::size_t>(sourceWidth) * static_cast<std::size_t>(sourceHeight),
                                      0u);
    std::vector<PixelCoord> queue;

    for (int y = 0; y < sourceHeight; ++y)
    {
        for (int x = 0; x < sourceWidth; ++x)
        {
            const std::size_t seedIndex =
                static_cast<std::size_t>(y) * static_cast<std::size_t>(sourceWidth) + static_cast<std::size_t>(x);
            if (visited[seedIndex] != 0u || alphaAt(sourcePixels, sourceWidth, x, y) == 0u)
            {
                continue;
            }

            Component component;
            queue.clear();
            queue.push_back({x, y});
            visited[seedIndex] = 1u;

            for (std::size_t cursor = 0; cursor < queue.size(); ++cursor)
            {
                const PixelCoord pixel = queue[cursor];
                component.pixels.push_back(pixel);
                component.minX = std::min(component.minX, pixel.x);
                component.minY = std::min(component.minY, pixel.y);
                component.maxX = std::max(component.maxX, pixel.x + 1);
                component.maxY = std::max(component.maxY, pixel.y + 1);

                constexpr std::array<PixelCoord, 4> NEIGHBORS = {{
                    {1, 0},
                    {-1, 0},
                    {0, 1},
                    {0, -1},
                }};

                for (const PixelCoord delta : NEIGHBORS)
                {
                    const int nextX = pixel.x + delta.x;
                    const int nextY = pixel.y + delta.y;
                    if (nextX < 0 || nextX >= sourceWidth || nextY < 0 || nextY >= sourceHeight)
                    {
                        continue;
                    }

                    const std::size_t nextIndex =
                        static_cast<std::size_t>(nextY) * static_cast<std::size_t>(sourceWidth) +
                        static_cast<std::size_t>(nextX);
                    if (visited[nextIndex] != 0u || alphaAt(sourcePixels, sourceWidth, nextX, nextY) == 0u)
                    {
                        continue;
                    }

                    visited[nextIndex] = 1u;
                    queue.push_back({nextX, nextY});
                }
            }

            if (static_cast<int>(component.pixels.size()) >= COMPONENT_MIN_AREA)
            {
                components.push_back(finalizeComponent(std::move(component)));
            }
        }
    }

    return components;
}

[[nodiscard]] std::vector<std::vector<int>> groupAnchorRows(const std::vector<Component>& components,
                                                            const std::vector<int>& anchorIndices)
{
    if (anchorIndices.empty())
    {
        return {};
    }

    float totalHeight = 0.0f;
    for (const int index : anchorIndices)
    {
        totalHeight += static_cast<float>(components[index].maxY - components[index].minY);
    }
    const float averageHeight = totalHeight / static_cast<float>(anchorIndices.size());
    const float rowThreshold = averageHeight * 0.45f;

    std::vector<int> sortedAnchors = anchorIndices;
    std::sort(sortedAnchors.begin(), sortedAnchors.end(),
              [&components](int lhs, int rhs)
              {
                  const Component& a = components[lhs];
                  const Component& b = components[rhs];
                  if (a.centerY == b.centerY)
                  {
                      return a.centerX < b.centerX;
                  }
                  return a.centerY < b.centerY;
              });

    std::vector<std::vector<int>> rows;
    for (const int anchorIndex : sortedAnchors)
    {
        if (rows.empty())
        {
            rows.push_back({anchorIndex});
            continue;
        }

        std::vector<int>& currentRow = rows.back();
        float rowCenterY = 0.0f;
        for (const int rowAnchorIndex : currentRow)
        {
            rowCenterY += components[rowAnchorIndex].centerY;
        }
        rowCenterY /= static_cast<float>(currentRow.size());

        if (std::abs(components[anchorIndex].centerY - rowCenterY) <= rowThreshold)
        {
            currentRow.push_back(anchorIndex);
        }
        else
        {
            rows.push_back({anchorIndex});
        }
    }

    for (std::vector<int>& row : rows)
    {
        std::sort(row.begin(), row.end(),
                  [&components](int lhs, int rhs) { return components[lhs].centerX < components[rhs].centerX; });
    }

    return rows;
}

[[nodiscard]] std::vector<int> selectAnchorIndices(const std::vector<Component>& components)
{
    std::vector<int> anchorIndices;
    for (int index = 0; index < static_cast<int>(components.size()); ++index)
    {
        if (components[index].area >= ANCHOR_MIN_AREA)
        {
            anchorIndices.push_back(index);
        }
    }

    const std::vector<std::vector<int>> rows = groupAnchorRows(components, anchorIndices);

    std::vector<int> selected;
    selected.reserve(FRAME_COUNT);
    for (const std::vector<int>& row : rows)
    {
        selected.insert(selected.end(), row.begin(), row.end());
    }

    if (static_cast<int>(selected.size()) != FRAME_COUNT)
    {
        throw std::runtime_error("[Duck] تعذر استخراج 11 فريمًا من duck.png بعد ترتيب المكونات الرئيسية.");
    }

    return selected;
}

[[nodiscard]] std::vector<std::vector<int>> assignComponentsToAnchors(const std::vector<Component>& components,
                                                                      const std::vector<int>& anchorIndices)
{
    std::vector<std::vector<int>> frameComponents(anchorIndices.size());
    std::vector<std::uint8_t> isAnchor(components.size(), 0u);

    for (int frameIndex = 0; frameIndex < static_cast<int>(anchorIndices.size()); ++frameIndex)
    {
        const int anchorIndex = anchorIndices[frameIndex];
        isAnchor[anchorIndex] = 1u;
        frameComponents[frameIndex].push_back(anchorIndex);
    }

    for (int componentIndex = 0; componentIndex < static_cast<int>(components.size()); ++componentIndex)
    {
        if (isAnchor[componentIndex] != 0u)
        {
            continue;
        }

        const Component& component = components[componentIndex];
        float bestDistanceSq = std::numeric_limits<float>::max();
        int bestFrameIndex = -1;

        for (int frameIndex = 0; frameIndex < static_cast<int>(anchorIndices.size()); ++frameIndex)
        {
            const Component& anchor = components[anchorIndices[frameIndex]];
            if (component.centerX < static_cast<float>(anchor.minX - ANCHOR_MERGE_MARGIN_X) ||
                component.centerX > static_cast<float>(anchor.maxX + ANCHOR_MERGE_MARGIN_X) ||
                component.centerY < static_cast<float>(anchor.minY - ANCHOR_MERGE_MARGIN_Y) ||
                component.centerY > static_cast<float>(anchor.maxY + ANCHOR_MERGE_MARGIN_Y))
            {
                continue;
            }

            const float dx = component.centerX - anchor.centerX;
            const float dy = component.centerY - anchor.centerY;
            const float distanceSq = dx * dx + dy * dy;
            if (distanceSq < bestDistanceSq)
            {
                bestDistanceSq = distanceSq;
                bestFrameIndex = frameIndex;
            }
        }

        if (bestFrameIndex >= 0)
        {
            frameComponents[bestFrameIndex].push_back(componentIndex);
        }
    }

    return frameComponents;
}

[[nodiscard]] RawFrame composeFrame(const unsigned char* sourcePixels, int sourceWidth,
                                    const std::vector<Component>& components, const std::vector<int>& componentIndices)
{
    if (componentIndices.empty())
    {
        return {};
    }

    int minX = std::numeric_limits<int>::max();
    int minY = std::numeric_limits<int>::max();
    int maxX = 0;
    int maxY = 0;

    for (const int index : componentIndices)
    {
        const Component& component = components[index];
        minX = std::min(minX, component.minX);
        minY = std::min(minY, component.minY);
        maxX = std::max(maxX, component.maxX);
        maxY = std::max(maxY, component.maxY);
    }

    RawFrame frame;
    frame.width = maxX - minX;
    frame.height = maxY - minY;
    frame.pixels.assign(static_cast<std::size_t>(frame.width) * static_cast<std::size_t>(frame.height) * 4u, 0u);

    for (const int index : componentIndices)
    {
        const Component& component = components[index];
        for (const PixelCoord pixel : component.pixels)
        {
            const std::size_t srcOffset = pixelOffset(sourceWidth, pixel.x, pixel.y);
            const std::size_t dstOffset = pixelOffset(frame.width, pixel.x - minX, pixel.y - minY);
            frame.pixels[dstOffset + 0u] = sourcePixels[srcOffset + 0u];
            frame.pixels[dstOffset + 1u] = sourcePixels[srcOffset + 1u];
            frame.pixels[dstOffset + 2u] = sourcePixels[srcOffset + 2u];
            frame.pixels[dstOffset + 3u] = sourcePixels[srcOffset + 3u];
        }
    }

    return frame;
}

[[nodiscard]] FrameSize fitSize(int width, int height, int maxWidth, int maxHeight)
{
    if (width <= 0 || height <= 0 || maxWidth <= 0 || maxHeight <= 0)
    {
        return {};
    }

    const float scale = std::min(static_cast<float>(maxWidth) / static_cast<float>(width),
                                 static_cast<float>(maxHeight) / static_cast<float>(height));

    return FrameSize{
        .width = std::max(1, static_cast<int>(std::lround(static_cast<float>(width) * scale))),
        .height = std::max(1, static_cast<int>(std::lround(static_cast<float>(height) * scale))),
    };
}

[[nodiscard]] std::vector<unsigned char> resizeFrameBilinear(const RawFrame& frame, int targetWidth, int targetHeight)
{
    if (frame.width <= 0 || frame.height <= 0 || targetWidth <= 0 || targetHeight <= 0)
    {
        return {};
    }

    if (frame.width == targetWidth && frame.height == targetHeight)
    {
        return frame.pixels;
    }

    std::vector<unsigned char> resized(
        static_cast<std::size_t>(targetWidth) * static_cast<std::size_t>(targetHeight) * 4u, 0u);

    for (int y = 0; y < targetHeight; ++y)
    {
        const float sourceY =
            ((static_cast<float>(y) + 0.5f) * static_cast<float>(frame.height) / static_cast<float>(targetHeight)) -
            0.5f;
        const int y0 = std::clamp(static_cast<int>(std::floor(sourceY)), 0, frame.height - 1);
        const int y1 = std::clamp(y0 + 1, 0, frame.height - 1);
        const float fy = std::clamp(sourceY - static_cast<float>(y0), 0.0f, 1.0f);

        for (int x = 0; x < targetWidth; ++x)
        {
            const float sourceX =
                ((static_cast<float>(x) + 0.5f) * static_cast<float>(frame.width) / static_cast<float>(targetWidth)) -
                0.5f;
            const int x0 = std::clamp(static_cast<int>(std::floor(sourceX)), 0, frame.width - 1);
            const int x1 = std::clamp(x0 + 1, 0, frame.width - 1);
            const float fx = std::clamp(sourceX - static_cast<float>(x0), 0.0f, 1.0f);

            const std::size_t dstOffset = pixelOffset(targetWidth, x, y);
            const std::size_t p00 = pixelOffset(frame.width, x0, y0);
            const std::size_t p10 = pixelOffset(frame.width, x1, y0);
            const std::size_t p01 = pixelOffset(frame.width, x0, y1);
            const std::size_t p11 = pixelOffset(frame.width, x1, y1);

            for (std::size_t channel = 0; channel < 4u; ++channel)
            {
                const float top = static_cast<float>(frame.pixels[p00 + channel]) * (1.0f - fx) +
                                  static_cast<float>(frame.pixels[p10 + channel]) * fx;
                const float bottom = static_cast<float>(frame.pixels[p01 + channel]) * (1.0f - fx) +
                                     static_cast<float>(frame.pixels[p11 + channel]) * fx;
                const float value = top * (1.0f - fy) + bottom * fy;
                resized[dstOffset + channel] = static_cast<unsigned char>(std::clamp(std::lround(value), 0L, 255L));
            }
        }
    }

    return resized;
}

void blitFrame(std::vector<unsigned char>& atlasPixels, int atlasWidth, int atlasHeight,
               const std::vector<unsigned char>& framePixels, int frameWidth, int frameHeight, int dstX, int dstY)
{
    if (framePixels.empty())
    {
        return;
    }

    for (int y = 0; y < frameHeight; ++y)
    {
        for (int x = 0; x < frameWidth; ++x)
        {
            const int atlasX = dstX + x;
            const int atlasY = dstY + y;
            if (atlasX < 0 || atlasX >= atlasWidth || atlasY < 0 || atlasY >= atlasHeight)
            {
                continue;
            }

            const std::size_t srcOffset = pixelOffset(frameWidth, x, y);
            const std::size_t dstOffset = pixelOffset(atlasWidth, atlasX, atlasY);
            atlasPixels[dstOffset + 0u] = framePixels[srcOffset + 0u];
            atlasPixels[dstOffset + 1u] = framePixels[srcOffset + 1u];
            atlasPixels[dstOffset + 2u] = framePixels[srcOffset + 2u];
            atlasPixels[dstOffset + 3u] = framePixels[srcOffset + 3u];
        }
    }
}

[[nodiscard]] std::vector<RawFrame> extractSourceFrames(const unsigned char* sourcePixels, int sourceWidth,
                                                        int sourceHeight)
{
    const std::vector<Component> components = extractComponents(sourcePixels, sourceWidth, sourceHeight);
    const std::vector<int> anchorIndices = selectAnchorIndices(components);
    const std::vector<std::vector<int>> frameComponents = assignComponentsToAnchors(components, anchorIndices);

    std::vector<RawFrame> frames;
    frames.reserve(frameComponents.size());
    for (const std::vector<int>& componentIndices : frameComponents)
    {
        frames.push_back(composeFrame(sourcePixels, sourceWidth, components, componentIndices));
    }

    if (static_cast<int>(frames.size()) != FRAME_COUNT)
    {
        throw std::runtime_error("[Duck] عدد فريمات البطة المستخرجة غير صالح.");
    }

    return frames;
}

[[maybe_unused]] [[nodiscard]] std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

[[maybe_unused]] [[nodiscard]] int extractOrderNumber(const std::filesystem::path& filePath)
{
    const std::string name = filePath.filename().string();
    int value = 0;
    bool foundDigit = false;

    for (const unsigned char ch : name)
    {
        if (!std::isdigit(ch))
        {
            if (foundDigit)
            {
                break;
            }
            continue;
        }

        foundDigit = true;
        value = value * 10 + static_cast<int>(ch - '0');
    }

    return foundDigit ? value : std::numeric_limits<int>::max();
}

[[maybe_unused]] [[nodiscard]] ReferenceFrame loadReferenceFrame(const std::filesystem::path& filePath)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(filePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr)
    {
        throw std::runtime_error("[Duck] فشل تحميل فريم مرجعي: " + filePath.string());
    }

    int minX = width;
    int minY = height;
    int maxX = 0;
    int maxY = 0;
    bool hasVisiblePixel = false;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            if (alphaAt(pixels, width, x, y) == 0u)
            {
                continue;
            }

            hasVisiblePixel = true;
            minX = std::min(minX, x);
            minY = std::min(minY, y);
            maxX = std::max(maxX, x + 1);
            maxY = std::max(maxY, y + 1);
        }
    }

    stbi_image_free(pixels);

    if (!hasVisiblePixel)
    {
        return ReferenceFrame{
            .canvasWidth = width,
            .canvasHeight = height,
            .bboxX = 0,
            .bboxY = 0,
            .bboxWidth = width,
            .bboxHeight = height,
        };
    }

    return ReferenceFrame{
        .canvasWidth = width,
        .canvasHeight = height,
        .bboxX = minX,
        .bboxY = minY,
        .bboxWidth = maxX - minX,
        .bboxHeight = maxY - minY,
    };
}

[[nodiscard]] std::vector<ReferenceFrame> loadReferenceFrames(const std::filesystem::path& sourceImagePath)
{
    int sourceWidth = 0;
    int sourceHeight = 0;
    int sourceChannels = 0;
    stbi_uc* sourcePixels =
        stbi_load(sourceImagePath.string().c_str(), &sourceWidth, &sourceHeight, &sourceChannels, STBI_rgb_alpha);

    if (sourcePixels == nullptr)
    {
        throw std::runtime_error("[Duck] فشل تحميل صورة المرجع: " + sourceImagePath.string());
    }

    const std::vector<RawFrame> sourceFrames = extractSourceFrames(sourcePixels, sourceWidth, sourceHeight);
    stbi_image_free(sourcePixels);
    return makeFallbackReferenceFrames(sourceFrames);
}

[[nodiscard]] float resolveReferenceScale(const std::vector<ReferenceFrame>& referenceFrames)
{
    int maxWidth = 0;
    int maxHeight = 0;
    for (const ReferenceFrame& frame : referenceFrames)
    {
        maxWidth = std::max(maxWidth, frame.canvasWidth);
        maxHeight = std::max(maxHeight, frame.canvasHeight);
    }

    if (maxWidth <= 0 || maxHeight <= 0)
    {
        throw std::runtime_error("[Duck] مقاسات فريمات البطة المرجعية غير صالحة.");
    }

    const float scale = std::min(static_cast<float>(OUTPUT_SAFE_WIDTH) / static_cast<float>(maxWidth),
                                 static_cast<float>(OUTPUT_SAFE_HEIGHT) / static_cast<float>(maxHeight));
    return std::min(1.0f, scale);
}

[[nodiscard]] ReferenceFrame scaleReferenceFrame(const ReferenceFrame& frame, float scale)
{
    const int scaledCanvasWidth =
        std::max(1, static_cast<int>(std::lround(static_cast<float>(frame.canvasWidth) * scale)));
    const int scaledCanvasHeight =
        std::max(1, static_cast<int>(std::lround(static_cast<float>(frame.canvasHeight) * scale)));

    const int scaledMinX =
        std::clamp(static_cast<int>(std::lround(static_cast<float>(frame.bboxX) * scale)), 0, scaledCanvasWidth - 1);
    const int scaledMinY =
        std::clamp(static_cast<int>(std::lround(static_cast<float>(frame.bboxY) * scale)), 0, scaledCanvasHeight - 1);
    const int scaledMaxX =
        std::clamp(static_cast<int>(std::lround(static_cast<float>(frame.bboxX + frame.bboxWidth) * scale)),
                   scaledMinX + 1, scaledCanvasWidth);
    const int scaledMaxY =
        std::clamp(static_cast<int>(std::lround(static_cast<float>(frame.bboxY + frame.bboxHeight) * scale)),
                   scaledMinY + 1, scaledCanvasHeight);

    return ReferenceFrame{
        .canvasWidth = scaledCanvasWidth,
        .canvasHeight = scaledCanvasHeight,
        .bboxX = scaledMinX,
        .bboxY = scaledMinY,
        .bboxWidth = scaledMaxX - scaledMinX,
        .bboxHeight = scaledMaxY - scaledMinY,
    };
}

[[nodiscard]] std::vector<ReferenceFrame> scaleReferenceFrames(const std::vector<ReferenceFrame>& referenceFrames)
{
    const float scale = resolveReferenceScale(referenceFrames);

    std::vector<ReferenceFrame> scaledFrames;
    scaledFrames.reserve(referenceFrames.size());
    for (const ReferenceFrame& frame : referenceFrames)
    {
        scaledFrames.push_back(scaleReferenceFrame(frame, scale));
    }

    return scaledFrames;
}

[[nodiscard]] std::vector<ReferenceFrame> makeFallbackReferenceFrames(const std::vector<RawFrame>& sourceFrames)
{
    std::vector<ReferenceFrame> referenceFrames;
    referenceFrames.reserve(sourceFrames.size());

    for (const RawFrame& frame : sourceFrames)
    {
        referenceFrames.push_back(ReferenceFrame{
            .canvasWidth = frame.width,
            .canvasHeight = frame.height,
            .bboxX = 0,
            .bboxY = 0,
            .bboxWidth = frame.width,
            .bboxHeight = frame.height,
        });
    }

    return referenceFrames;
}

[[nodiscard]] std::vector<PreparedFrame> prepareFrames(const std::vector<RawFrame>& sourceFrames,
                                                       const std::vector<ReferenceFrame>& referenceFrames)
{
    if (sourceFrames.size() != referenceFrames.size() || static_cast<int>(sourceFrames.size()) != FRAME_COUNT)
    {
        throw std::runtime_error("[Duck] بيانات فريمات البطة غير مكتملة.");
    }

    std::vector<PreparedFrame> preparedFrames;
    preparedFrames.reserve(sourceFrames.size());

    int unifiedSourceWidth = 0;
    int unifiedSourceHeight = 0;
    for (const ReferenceFrame& referenceFrame : referenceFrames)
    {
        unifiedSourceWidth = std::max(unifiedSourceWidth, referenceFrame.bboxWidth);
        unifiedSourceHeight = std::max(unifiedSourceHeight, referenceFrame.bboxHeight);
    }

    if (unifiedSourceWidth <= 0 || unifiedSourceHeight <= 0)
    {
        throw std::runtime_error("[Duck] أبعاد الفريمات المرجعية الموحّدة غير صالحة.");
    }

    for (std::size_t index = 0; index < sourceFrames.size(); ++index)
    {
        const RawFrame& sourceFrame = sourceFrames[index];
        const FrameSize targetSize =
            fitSize(sourceFrame.width, sourceFrame.height, unifiedSourceWidth, unifiedSourceHeight);

        if (targetSize.width <= 0 || targetSize.height <= 0)
        {
            throw std::runtime_error("[Duck] تعذر تحديد حجم فريم البطة بعد المطابقة المرجعية.");
        }

        PreparedFrame prepared;
        prepared.width = targetSize.width;
        prepared.height = targetSize.height;
        prepared.sourceW = unifiedSourceWidth;
        prepared.sourceH = unifiedSourceHeight;
        prepared.spriteX = (unifiedSourceWidth - prepared.width) / 2;
        prepared.spriteY = unifiedSourceHeight - prepared.height;
        prepared.pixels = resizeFrameBilinear(sourceFrame, prepared.width, prepared.height);
        preparedFrames.push_back(std::move(prepared));
    }

    return preparedFrames;
}

void addClip(SpriteAtlasData& data, const std::string& key, std::initializer_list<int> frameIndices, bool loop)
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

void addDirectionalPair(SpriteAtlasData& data, const std::string& baseName, std::initializer_list<int> frameIndices,
                        bool loop)
{
    addClip(data, baseName + "_right", frameIndices, loop);
    addClip(data, baseName + "_left", frameIndices, loop);
}

[[nodiscard]] SpriteAtlasData buildDuckAtlasData(int imageWidth, int imageHeight, const std::vector<AtlasFrame>& frames)
{
    if (static_cast<int>(frames.size()) != FRAME_COUNT)
    {
        throw std::runtime_error("[Duck] عدد فريمات أطلس البطة غير صالح.");
    }

    SpriteAtlasData data;
    data.imageWidth = imageWidth;
    data.imageHeight = imageHeight;
    data.frames = frames;

    addDirectionalPair(data, "fly", {0, 1, 2, 3, 4, 5, 4, 3, 2, 1}, true);

    addDirectionalPair(data, "hit", {6, 7, 8, 9, 10}, false);

    // الحفاظ على المفاتيح القديمة كمرادفات، لأن الكود المحيط قد يطلبها لاحقًا.
    addDirectionalPair(data, "fall", {6, 7, 8, 9, 10}, false);

    addDirectionalPair(data, "dead", {10}, false);

    addDirectionalPair(data, "dead_idle", {10}, true);

    return data;
}

[[nodiscard]] DuckAtlasSheet buildDuckAtlasSheet(const std::vector<RawFrame>& sourceFrames,
                                                 const std::vector<ReferenceFrame>& referenceFrames)
{
    const std::vector<PreparedFrame> preparedFrames = prepareFrames(sourceFrames, referenceFrames);

    int maxFrameWidth = 0;
    int maxFrameHeight = 0;
    for (const PreparedFrame& frame : preparedFrames)
    {
        maxFrameWidth = std::max(maxFrameWidth, frame.width);
        maxFrameHeight = std::max(maxFrameHeight, frame.height);
    }

    if (maxFrameWidth <= 0 || maxFrameHeight <= 0)
    {
        throw std::runtime_error("[Duck] تعذر تحديد أبعاد فريمات أطلس البطة.");
    }

    const int cellWidth = maxFrameWidth + CELL_PADDING * 2;
    const int cellHeight = maxFrameHeight + CELL_PADDING * 2;
    const int frameRows = (FRAME_COUNT + FRAME_COLUMNS - 1) / FRAME_COLUMNS;

    DuckAtlasSheet sheet;
    sheet.imageWidth = FRAME_COLUMNS * cellWidth;
    sheet.imageHeight = frameRows * cellHeight;
    sheet.pixels.assign(static_cast<std::size_t>(sheet.imageWidth) * static_cast<std::size_t>(sheet.imageHeight) * 4u,
                        0u);

    std::vector<AtlasFrame> atlasFrames;
    atlasFrames.reserve(preparedFrames.size());

    for (int frameIndex = 0; frameIndex < static_cast<int>(preparedFrames.size()); ++frameIndex)
    {
        const PreparedFrame& preparedFrame = preparedFrames[frameIndex];
        const int col = frameIndex % FRAME_COLUMNS;
        const int row = frameIndex / FRAME_COLUMNS;
        const int atlasX = col * cellWidth + CELL_PADDING + (maxFrameWidth - preparedFrame.width) / 2;
        const int atlasY = row * cellHeight + CELL_PADDING + (maxFrameHeight - preparedFrame.height) / 2;

        blitFrame(sheet.pixels, sheet.imageWidth, sheet.imageHeight, preparedFrame.pixels, preparedFrame.width,
                  preparedFrame.height, atlasX, atlasY);

        atlasFrames.push_back(AtlasFrame{
            .x = atlasX,
            .y = atlasY,
            .width = preparedFrame.width,
            .height = preparedFrame.height,
            .sourceW = preparedFrame.sourceW,
            .sourceH = preparedFrame.sourceH,
            .spriteX = preparedFrame.spriteX,
            .spriteY = preparedFrame.spriteY,
            .pivotX = PIVOT_X,
            .pivotY = PIVOT_Y,
            .durationMs = FRAME_DEFS[frameIndex].durationMs,
        });
    }

    sheet.atlas = buildDuckAtlasData(sheet.imageWidth, sheet.imageHeight, atlasFrames);
    return sheet;
}

} // namespace

SpriteAtlasData createDuckAtlasData(int imageWidth, int imageHeight, const std::vector<AtlasFrame>& frames)
{
    return buildDuckAtlasData(imageWidth, imageHeight, frames);
}

DuckAtlasSheet createDuckAtlasSheetFromPixels(const unsigned char* sourcePixels, int sourceWidth, int sourceHeight)
{
    if (sourcePixels == nullptr || sourceWidth <= 0 || sourceHeight <= 0)
    {
        throw std::runtime_error("[Duck] بيانات duck.png غير صالحة.");
    }

    const std::vector<RawFrame> sourceFrames = extractSourceFrames(sourcePixels, sourceWidth, sourceHeight);
    const std::vector<ReferenceFrame> fallbackReferenceFrames = makeFallbackReferenceFrames(sourceFrames);
    return buildDuckAtlasSheet(sourceFrames, fallbackReferenceFrames);
}

DuckAtlasSheet loadDuckAtlasSheetFromSourceImage(const std::string& sourceImagePath)
{
    int sourceWidth = 0;
    int sourceHeight = 0;
    int sourceChannels = 0;
    stbi_uc* sourcePixels =
        stbi_load(sourceImagePath.c_str(), &sourceWidth, &sourceHeight, &sourceChannels, STBI_rgb_alpha);

    if (sourcePixels == nullptr)
    {
        throw std::runtime_error("[Duck] فشل تحميل صورة البطة: " + sourceImagePath);
    }

    const std::vector<RawFrame> sourceFrames = extractSourceFrames(sourcePixels, sourceWidth, sourceHeight);
    stbi_image_free(sourcePixels);

    const std::vector<ReferenceFrame> referenceFrames =
        scaleReferenceFrames(loadReferenceFrames(std::filesystem::path(sourceImagePath)));
    return buildDuckAtlasSheet(sourceFrames, referenceFrames);
}

} // namespace game
