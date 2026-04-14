#include "BasicArabicShaper.h"

#include "ArabicJoiner.h"

#include <algorithm>

namespace textsys
{
namespace
{

[[nodiscard]] int findPreviousJoinCandidate(const std::u32string& text, int index)
{
    for (int i = index - 1; i >= 0; --i)
    {
        if (!arabic::isTransparentMark(text[static_cast<std::size_t>(i)]))
        {
            return i;
        }
    }

    return -1;
}

[[nodiscard]] int findNextJoinCandidate(const std::u32string& text, int index)
{
    for (int i = index + 1; i < static_cast<int>(text.size()); ++i)
    {
        if (!arabic::isTransparentMark(text[static_cast<std::size_t>(i)]))
        {
            return i;
        }
    }

    return -1;
}

[[nodiscard]] GlyphInfo makePassthroughGlyph(const TextRun& run, std::size_t localIndex, char32_t codepoint)
{
    GlyphInfo glyph;
    glyph.sourceCodepoint = codepoint;
    glyph.glyphCodepoint = codepoint;
    glyph.script = run.script;
    glyph.direction = run.direction;
    glyph.form = GlyphForm::None;
    glyph.clusterStart = run.logicalStart + localIndex;
    glyph.clusterLength = 1;
    glyph.visible = true;
    return glyph;
}

} // namespace

std::vector<GlyphInfo> BasicArabicShaper::shapeRun(const TextRun& run) const
{
    std::vector<GlyphInfo> glyphs;
    glyphs.reserve(run.text.size());

    if (run.script != Script::Arabic)
    {
        for (std::size_t i = 0; i < run.text.size(); ++i)
        {
            glyphs.push_back(makePassthroughGlyph(run, i, run.text[i]));
        }
        return glyphs;
    }

    const std::u32string& text = run.text;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        const char32_t codepoint = text[i];

        if (arabic::isTransparentMark(codepoint))
        {
            glyphs.push_back(makePassthroughGlyph(run, i, codepoint));
            continue;
        }

        const int previousIndex = findPreviousJoinCandidate(text, static_cast<int>(i));
        const int nextIndex = findNextJoinCandidate(text, static_cast<int>(i));

        const char32_t previousCodepoint = previousIndex >= 0 ? text[static_cast<std::size_t>(previousIndex)] : U'\0';
        const char32_t nextCodepoint = nextIndex >= 0 ? text[static_cast<std::size_t>(nextIndex)] : U'\0';

        const bool joinsPrevious = previousIndex >= 0 && arabic::canConnectLeft(previousCodepoint) &&
                                   arabic::canConnectRight(codepoint);
        const bool joinsNext =
            nextIndex >= 0 && arabic::canConnectLeft(codepoint) && arabic::canConnectRight(nextCodepoint);

        if (i + 1u < text.size())
        {
            char32_t ligatureCodepoint = U'\0';
            if (arabic::tryShapeLamAlef(codepoint, text[i + 1u], joinsPrevious, &ligatureCodepoint))
            {
                GlyphInfo glyph;
                glyph.sourceCodepoint = codepoint;
                glyph.glyphCodepoint = ligatureCodepoint;
                glyph.script = Script::Arabic;
                glyph.direction = TextDirection::RightToLeft;
                glyph.form = GlyphForm::Ligature;
                glyph.clusterStart = run.logicalStart + i;
                glyph.clusterLength = 2;
                glyph.visible = true;
                glyphs.push_back(glyph);
                ++i;
                continue;
            }
        }

        GlyphForm form = GlyphForm::Isolated;
        if (joinsPrevious && joinsNext)
        {
            form = GlyphForm::Medial;
        }
        else if (joinsPrevious)
        {
            form = GlyphForm::Final;
        }
        else if (joinsNext)
        {
            form = GlyphForm::Initial;
        }

        GlyphInfo glyph;
        glyph.sourceCodepoint = codepoint;
        glyph.glyphCodepoint = arabic::getPresentationForm(codepoint, form);
        glyph.script = Script::Arabic;
        glyph.direction = TextDirection::RightToLeft;
        glyph.form = form;
        glyph.clusterStart = run.logicalStart + i;
        glyph.clusterLength = 1;
        glyph.visible = true;
        glyphs.push_back(glyph);
    }

    return glyphs;
}

} // namespace textsys
