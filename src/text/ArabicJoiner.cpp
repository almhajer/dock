#include "ArabicJoiner.h"

#include <array>

namespace textsys::arabic
{
namespace
{

constexpr std::array<ArabicShapeEntry, 37> kShapeTable = {{
    {U'\u0621', JoiningType::NonJoining, U'\uFE80', U'\uFE80', U'\0', U'\0'},
    {U'\u0622', JoiningType::RightJoining, U'\uFE81', U'\uFE82', U'\0', U'\0'},
    {U'\u0623', JoiningType::RightJoining, U'\uFE83', U'\uFE84', U'\0', U'\0'},
    {U'\u0624', JoiningType::RightJoining, U'\uFE85', U'\uFE86', U'\0', U'\0'},
    {U'\u0625', JoiningType::RightJoining, U'\uFE87', U'\uFE88', U'\0', U'\0'},
    {U'\u0626', JoiningType::DualJoining, U'\uFE89', U'\uFE8A', U'\uFE8B', U'\uFE8C'},
    {U'\u0627', JoiningType::RightJoining, U'\uFE8D', U'\uFE8E', U'\0', U'\0'},
    {U'\u0628', JoiningType::DualJoining, U'\uFE8F', U'\uFE90', U'\uFE91', U'\uFE92'},
    {U'\u0629', JoiningType::RightJoining, U'\uFE93', U'\uFE94', U'\0', U'\0'},
    {U'\u062A', JoiningType::DualJoining, U'\uFE95', U'\uFE96', U'\uFE97', U'\uFE98'},
    {U'\u062B', JoiningType::DualJoining, U'\uFE99', U'\uFE9A', U'\uFE9B', U'\uFE9C'},
    {U'\u062C', JoiningType::DualJoining, U'\uFE9D', U'\uFE9E', U'\uFE9F', U'\uFEA0'},
    {U'\u062D', JoiningType::DualJoining, U'\uFEA1', U'\uFEA2', U'\uFEA3', U'\uFEA4'},
    {U'\u062E', JoiningType::DualJoining, U'\uFEA5', U'\uFEA6', U'\uFEA7', U'\uFEA8'},
    {U'\u062F', JoiningType::RightJoining, U'\uFEA9', U'\uFEAA', U'\0', U'\0'},
    {U'\u0630', JoiningType::RightJoining, U'\uFEAB', U'\uFEAC', U'\0', U'\0'},
    {U'\u0631', JoiningType::RightJoining, U'\uFEAD', U'\uFEAE', U'\0', U'\0'},
    {U'\u0632', JoiningType::RightJoining, U'\uFEAF', U'\uFEB0', U'\0', U'\0'},
    {U'\u0633', JoiningType::DualJoining, U'\uFEB1', U'\uFEB2', U'\uFEB3', U'\uFEB4'},
    {U'\u0634', JoiningType::DualJoining, U'\uFEB5', U'\uFEB6', U'\uFEB7', U'\uFEB8'},
    {U'\u0635', JoiningType::DualJoining, U'\uFEB9', U'\uFEBA', U'\uFEBB', U'\uFEBC'},
    {U'\u0636', JoiningType::DualJoining, U'\uFEBD', U'\uFEBE', U'\uFEBF', U'\uFEC0'},
    {U'\u0637', JoiningType::DualJoining, U'\uFEC1', U'\uFEC2', U'\uFEC3', U'\uFEC4'},
    {U'\u0638', JoiningType::DualJoining, U'\uFEC5', U'\uFEC6', U'\uFEC7', U'\uFEC8'},
    {U'\u0639', JoiningType::DualJoining, U'\uFEC9', U'\uFECA', U'\uFECB', U'\uFECC'},
    {U'\u063A', JoiningType::DualJoining, U'\uFECD', U'\uFECE', U'\uFECF', U'\uFED0'},
    {U'\u0640', JoiningType::DualJoining, U'\u0640', U'\u0640', U'\u0640', U'\u0640'},
    {U'\u0641', JoiningType::DualJoining, U'\uFED1', U'\uFED2', U'\uFED3', U'\uFED4'},
    {U'\u0642', JoiningType::DualJoining, U'\uFED5', U'\uFED6', U'\uFED7', U'\uFED8'},
    {U'\u0643', JoiningType::DualJoining, U'\uFED9', U'\uFEDA', U'\uFEDB', U'\uFEDC'},
    {U'\u0644', JoiningType::DualJoining, U'\uFEDD', U'\uFEDE', U'\uFEDF', U'\uFEE0'},
    {U'\u0645', JoiningType::DualJoining, U'\uFEE1', U'\uFEE2', U'\uFEE3', U'\uFEE4'},
    {U'\u0646', JoiningType::DualJoining, U'\uFEE5', U'\uFEE6', U'\uFEE7', U'\uFEE8'},
    {U'\u0647', JoiningType::DualJoining, U'\uFEE9', U'\uFEEA', U'\uFEEB', U'\uFEEC'},
    {U'\u0648', JoiningType::RightJoining, U'\uFEED', U'\uFEEE', U'\0', U'\0'},
    {U'\u0649', JoiningType::RightJoining, U'\uFEEF', U'\uFEF0', U'\0', U'\0'},
    {U'\u064A', JoiningType::DualJoining, U'\uFEF1', U'\uFEF2', U'\uFEF3', U'\uFEF4'},
}};

[[nodiscard]] bool isArabicPresentationForm(char32_t codepoint)
{
    return codepoint >= 0xFB50u && codepoint <= 0xFEFFu;
}

} // namespace

const ArabicShapeEntry* findShapeEntry(char32_t codepoint)
{
    for (const ArabicShapeEntry& entry : kShapeTable)
    {
        if (entry.baseCodepoint == codepoint)
        {
            return &entry;
        }
    }

    return nullptr;
}

bool isArabicLetter(char32_t codepoint)
{
    return findShapeEntry(codepoint) != nullptr || isArabicPresentationForm(codepoint);
}

bool isTransparentMark(char32_t codepoint)
{
    return (codepoint >= 0x064Bu && codepoint <= 0x065Fu) || codepoint == 0x0670u ||
           (codepoint >= 0x06D6u && codepoint <= 0x06EDu);
}

JoiningType getJoiningType(char32_t codepoint)
{
    if (isTransparentMark(codepoint))
    {
        return JoiningType::Transparent;
    }

    if (const ArabicShapeEntry* entry = findShapeEntry(codepoint))
    {
        return entry->joiningType;
    }

    return JoiningType::NonJoining;
}

bool canConnectRight(char32_t codepoint)
{
    const JoiningType joiningType = getJoiningType(codepoint);
    return joiningType == JoiningType::RightJoining || joiningType == JoiningType::DualJoining;
}

bool canConnectLeft(char32_t codepoint)
{
    return getJoiningType(codepoint) == JoiningType::DualJoining;
}

char32_t getPresentationForm(char32_t codepoint, GlyphForm form)
{
    const ArabicShapeEntry* entry = findShapeEntry(codepoint);
    if (entry == nullptr)
    {
        return codepoint;
    }

    switch (form)
    {
    case GlyphForm::Isolated:
        return entry->isolated != U'\0' ? entry->isolated : codepoint;
    case GlyphForm::Final:
        return entry->final != U'\0' ? entry->final : entry->isolated;
    case GlyphForm::Initial:
        return entry->initial != U'\0' ? entry->initial : entry->isolated;
    case GlyphForm::Medial:
        return entry->medial != U'\0' ? entry->medial : entry->isolated;
    default:
        return codepoint;
    }
}

bool tryShapeLamAlef(char32_t lamCodepoint, char32_t alefCodepoint, bool connectsToPrevious, char32_t* outGlyph)
{
    if (lamCodepoint != U'\u0644' || outGlyph == nullptr)
    {
        return false;
    }

    switch (alefCodepoint)
    {
    case U'\u0622':
        *outGlyph = connectsToPrevious ? U'\uFEF6' : U'\uFEF5';
        return true;
    case U'\u0623':
        *outGlyph = connectsToPrevious ? U'\uFEF8' : U'\uFEF7';
        return true;
    case U'\u0625':
        *outGlyph = connectsToPrevious ? U'\uFEFA' : U'\uFEF9';
        return true;
    case U'\u0627':
        *outGlyph = connectsToPrevious ? U'\uFEFC' : U'\uFEFB';
        return true;
    default:
        return false;
    }
}

} // namespace textsys::arabic
