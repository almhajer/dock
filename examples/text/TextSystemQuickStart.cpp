#include "text/TextSystem.h"

#include <iostream>
#include <string_view>

namespace
{

void dumpPreparedText(std::string_view label, const textsys::PreparedText& prepared)
{
    std::cout << "\n[" << label << "]\n";
    std::cout << "glyphCount = " << prepared.layout.glyphs.size() << '\n';
    std::cout << "quadCount = " << prepared.quads.size() << '\n';
    std::cout << "vertexCount = " << prepared.vertices.size() << '\n';
    std::cout << "lineWidth = " << prepared.layout.width << '\n';
}

} // namespace

int main()
{
    textsys::TextSystem textSystem(18.0f);

    textsys::TextLayoutOptions layoutOptions;
    layoutOptions.originX = 540.0f;
    layoutOptions.baselineY = 96.0f;
    layoutOptions.fontScale = 1.0f;
    layoutOptions.alignment = textsys::TextAlignment::End;

    textsys::TextRenderStyle renderStyle;
    renderStyle.color = {0.95f, 0.95f, 0.97f, 1.0f};

    dumpPreparedText(
        "stage-label",
        textSystem.prepareUtf8(u8"مرحلة 1", {.layout = layoutOptions, .render = renderStyle}));

    dumpPreparedText(
        "mixed-prompt",
        textSystem.prepareUtf8(u8"اضغط ENTER للفتح", {.layout = layoutOptions, .render = renderStyle}));

    dumpPreparedText(
        "ammo-hud",
        textSystem.prepareUtf8(u8"تلقيم 1/1", {.layout = layoutOptions, .render = renderStyle}));

    return 0;
}
