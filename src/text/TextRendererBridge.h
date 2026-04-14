#pragma once

#include "TextRendererTypes.h"
#include "TextTypes.h"

namespace textsys
{

/*
 * جسر التحويل من TextLayout إلى quads مستقلة عن Vulkan.
 */
class TextRendererBridge
{
  public:
    [[nodiscard]] static std::vector<GlyphQuad> buildGlyphQuads(const TextLayout& layout,
                                                                const TextRenderStyle& style = {});
    [[nodiscard]] static std::vector<TextVertex> flattenVertices(const std::vector<GlyphQuad>& quads);
};

} // namespace textsys
