#include "BasicBidiResolver.h"

#include "ArabicJoiner.h"

#include <algorithm>

namespace textsys
{
namespace
{

[[nodiscard]] bool isAsciiLatin(char32_t codepoint)
{
    return (codepoint >= U'a' && codepoint <= U'z') || (codepoint >= U'A' && codepoint <= U'Z');
}

[[nodiscard]] bool isArabicDigit(char32_t codepoint)
{
    return (codepoint >= U'\u0660' && codepoint <= U'\u0669') || (codepoint >= U'\u06F0' && codepoint <= U'\u06F9');
}

[[nodiscard]] bool isDigit(char32_t codepoint)
{
    return (codepoint >= U'0' && codepoint <= U'9') || isArabicDigit(codepoint);
}

[[nodiscard]] bool isWhitespace(char32_t codepoint)
{
    return codepoint == U' ' || codepoint == U'\t' || codepoint == U'\n' || codepoint == U'\r';
}

[[nodiscard]] bool isNumericCompanion(char32_t codepoint)
{
    switch (codepoint)
    {
    case U'/':
    case U'+':
    case U'-':
    case U'%':
    case U'.':
    case U',':
    case U':':
    case U'\u066A':
    case U'\u066B':
    case U'\u066C':
        return true;
    default:
        return false;
    }
}

[[nodiscard]] CharacterClass classifyCodepoint(char32_t codepoint)
{
    if (isWhitespace(codepoint))
    {
        return CharacterClass::Whitespace;
    }
    if (arabic::isArabicLetter(codepoint) || arabic::isTransparentMark(codepoint))
    {
        return CharacterClass::Arabic;
    }
    if (isAsciiLatin(codepoint))
    {
        return CharacterClass::Latin;
    }
    if (isDigit(codepoint))
    {
        return CharacterClass::Digit;
    }

    return CharacterClass::Neutral;
}

[[nodiscard]] Script classToScript(CharacterClass klass)
{
    switch (klass)
    {
    case CharacterClass::Arabic:
        return Script::Arabic;
    case CharacterClass::Latin:
        return Script::Latin;
    case CharacterClass::Digit:
        return Script::Digit;
    case CharacterClass::Whitespace:
        return Script::Whitespace;
    case CharacterClass::Neutral:
    default:
        return Script::Neutral;
    }
}

[[nodiscard]] TextDirection classToDirection(CharacterClass klass)
{
    switch (klass)
    {
    case CharacterClass::Arabic:
        return TextDirection::RightToLeft;
    case CharacterClass::Latin:
    case CharacterClass::Digit:
        return TextDirection::LeftToRight;
    case CharacterClass::Whitespace:
    case CharacterClass::Neutral:
    default:
        return TextDirection::Neutral;
    }
}

[[nodiscard]] bool isStrongRun(const TextRun& run)
{
    return run.script == Script::Arabic || run.script == Script::Latin;
}

[[nodiscard]] std::vector<TextRun> buildLogicalRuns(const UnicodeText& text)
{
    std::vector<TextRun> runs;
    const std::u32string& codepoints = text.codepoints;

    std::size_t index = 0;
    while (index < codepoints.size())
    {
        const char32_t current = codepoints[index];
        const CharacterClass klass = classifyCodepoint(current);

        TextRun run;
        run.logicalStart = index;
        run.script = classToScript(klass);
        run.direction = classToDirection(klass);

        if (klass == CharacterClass::Whitespace)
        {
            while (index < codepoints.size() && classifyCodepoint(codepoints[index]) == CharacterClass::Whitespace)
            {
                run.text.push_back(codepoints[index]);
                ++index;
            }
        }
        else if (klass == CharacterClass::Digit)
        {
            while (index < codepoints.size())
            {
                const char32_t codepoint = codepoints[index];
                if (isDigit(codepoint) || isNumericCompanion(codepoint))
                {
                    run.text.push_back(codepoint);
                    ++index;
                    continue;
                }
                break;
            }
        }
        else if (klass == CharacterClass::Latin)
        {
            while (index < codepoints.size())
            {
                const char32_t codepoint = codepoints[index];
                if (isAsciiLatin(codepoint) || isDigit(codepoint) || codepoint == U'_')
                {
                    run.text.push_back(codepoint);
                    ++index;
                    continue;
                }
                break;
            }
        }
        else if (klass == CharacterClass::Arabic)
        {
            while (index < codepoints.size())
            {
                const char32_t codepoint = codepoints[index];
                if (arabic::isArabicLetter(codepoint) || arabic::isTransparentMark(codepoint))
                {
                    run.text.push_back(codepoint);
                    ++index;
                    continue;
                }
                break;
            }
        }
        else
        {
            while (index < codepoints.size())
            {
                const CharacterClass nextClass = classifyCodepoint(codepoints[index]);
                if (nextClass == CharacterClass::Neutral)
                {
                    run.text.push_back(codepoints[index]);
                    ++index;
                    continue;
                }
                break;
            }
        }

        run.logicalLength = run.text.size();
        run.embeddingLevel = run.direction == TextDirection::RightToLeft ? 1u : 0u;
        runs.push_back(std::move(run));
    }

    return runs;
}

} // namespace

TextDirection BasicBidiResolver::resolveBaseDirection(const UnicodeText& text) const
{
    for (char32_t codepoint : text.codepoints)
    {
        const CharacterClass klass = classifyCodepoint(codepoint);
        if (klass == CharacterClass::Arabic)
        {
            return TextDirection::RightToLeft;
        }
        if (klass == CharacterClass::Latin)
        {
            return TextDirection::LeftToRight;
        }
    }

    return TextDirection::LeftToRight;
}

std::vector<TextRun> BasicBidiResolver::resolveVisualRuns(const UnicodeText& text) const
{
    std::vector<TextRun> runs = buildLogicalRuns(text);
    if (runs.empty())
    {
        return runs;
    }

    const TextDirection baseDirection = resolveBaseDirection(text);
    if (baseDirection != TextDirection::RightToLeft)
    {
        return runs;
    }

    auto firstStrongIt = std::find_if(runs.begin(), runs.end(), [](const TextRun& run) { return isStrongRun(run); });
    if (firstStrongIt == runs.end() || firstStrongIt->script != Script::Arabic)
    {
        return runs;
    }

    std::vector<TextRun> visualRuns;
    visualRuns.reserve(runs.size());

    visualRuns.insert(visualRuns.end(), runs.begin(), firstStrongIt);
    std::reverse_copy(firstStrongIt, runs.end(), std::back_inserter(visualRuns));
    return visualRuns;
}

} // namespace textsys
