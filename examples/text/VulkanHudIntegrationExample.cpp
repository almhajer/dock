#include "text/TextSystem.h"

#include <cstddef>
#include <vector>

namespace samples
{

struct HudTextUploadPacket
{
    std::vector<textsys::TextVertex> vertices;
    std::vector<textsys::GlyphQuad> quads;
    float width = 0.0f;
    float baseline = 0.0f;
};

/*
 * مثال استعمال داخل HUD:
 * - نبني النص العربي المختلط مرة واحدة
 * - ثم نرفع vertices الناتجة إلى الـ vertex buffer الحالي
 */
HudTextUploadPacket buildAmmoPacket()
{
    textsys::TextSystem textSystem(22.0f);

    textsys::TextLayoutOptions layoutOptions;
    layoutOptions.originX = 1180.0f;
    layoutOptions.baselineY = 86.0f;
    layoutOptions.alignment = textsys::TextAlignment::End;

    textsys::TextRenderStyle renderStyle;
    renderStyle.color = {1.0f, 1.0f, 1.0f, 1.0f};

    const textsys::PreparedText prepared =
        textSystem.prepareUtf8(u8"طلقة 460+", {.layout = layoutOptions, .render = renderStyle});

    HudTextUploadPacket packet;
    packet.vertices = prepared.vertices;
    packet.quads = prepared.quads;
    packet.width = prepared.layout.width;
    packet.baseline = prepared.layout.baseline;
    return packet;
}

/*
 * هذه الدالة توضح فقط نقطة الدمج.
 * انسخ vertices إلى staging buffer أو مباشرة إلى dynamic vertex buffer حسب renderer الحالي.
 */
void uploadHudTextToVulkanBuffer(const HudTextUploadPacket& packet, void* mappedVertexMemory)
{
    if (mappedVertexMemory == nullptr || packet.vertices.empty())
    {
        return;
    }

    auto* destination = static_cast<textsys::TextVertex*>(mappedVertexMemory);
    for (std::size_t i = 0; i < packet.vertices.size(); ++i)
    {
        destination[i] = packet.vertices[i];
    }
}

} // namespace samples
