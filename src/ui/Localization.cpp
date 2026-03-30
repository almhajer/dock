#include "Localization.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace ui {

bool Localization::load(const std::string& assetsPath, Language lang) {
    mCurrentLang = lang;

    // بناء مسار الملف: assets/lang/ar.json أو assets/lang/en.json
    std::string code = (lang == Language::Arabic) ? "ar" : "en";
    std::string filePath = assetsPath + "/lang/" + code + ".json";

    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    // قراءة كامل المحتوى
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    return parseJSON(content);
}

const std::string& Localization::get(const std::string& key) const {
    static const std::string empty;
    auto it = mStrings.find(key);
    if (it != mStrings.end()) {
        return it->second;
    }
    // إرجاع المفتاح نفسه إذا لم يُوجد - يساعد في اكتشاف النصوص المفقودة
    return key;
}

std::string Localization::getFormatted(const std::string& key, int value) const {
    std::string text = get(key);
    std::string valStr = std::to_string(value);

    // استبدال كل {n} بالقيمة
    size_t pos = 0;
    while ((pos = text.find("{n}", pos)) != std::string::npos) {
        text.replace(pos, 3, valStr);
        pos += valStr.size();
    }
    return text;
}

void Localization::setLanguage(const std::string& assetsPath, Language lang) {
    load(assetsPath, lang);
}

Localization::Language Localization::getCurrentLanguage() const {
    return mCurrentLang;
}

std::string_view Localization::getLanguageCode() const {
    return (mCurrentLang == Language::Arabic) ? "ar" : "en";
}

bool Localization::isRTL() const {
    return mCurrentLang == Language::Arabic;
}

// ─── تحليل JSON بسيط (يدعم {"key": "value"} فقط) ─────────────────

bool Localization::parseJSON(const std::string& content) {
    mStrings.clear();

    size_t i = 0;
    auto skipWhitespace = [&]() {
        while (i < content.size() && (content[i] == ' ' || content[i] == '\t' ||
               content[i] == '\n' || content[i] == '\r')) {
            i++;
        }
    };

    auto parseString = [&]() -> std::string {
        if (i >= content.size() || content[i] != '"') return "";
        i++; // تخطي علامة الافتتاح "

        std::string result;
        while (i < content.size() && content[i] != '"') {
            if (content[i] == '\\' && i + 1 < content.size()) {
                i++; // تخطي حرف الهروب
                switch (content[i]) {
                    case '"':  result += '"';  break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/';  break;
                    case 'n':  result += '\n'; break;
                    case 't':  result += '\t'; break;
                    case 'u':  // تخطي رموز Unicode المؤلفة من 4 أرقام
                        result += "\\u";
                        for (int j = 0; j < 4 && i + 1 < content.size(); j++) {
                            i++;
                            result += content[i];
                        }
                        break;
                    default: result += content[i]; break;
                }
            } else {
                result += content[i];
            }
            i++;
        }
        if (i < content.size()) i++; // تخطي علامة الإغلاق "
        return result;
    };

    skipWhitespace();
    if (i >= content.size() || content[i] != '{') return false;
    i++; // تخطي {

    while (i < content.size()) {
        skipWhitespace();
        if (i >= content.size()) break;
        if (content[i] == '}') break; // نهاية الكائن

        // قراءة المفتاح
        std::string key = parseString();
        if (key.empty()) { i++; continue; }

        skipWhitespace();
        if (i >= content.size() || content[i] != ':') return false;
        i++; // تخطي :

        skipWhitespace();

        // قراءة القيمة
        std::string value = parseString();
        mStrings[key] = value;

        skipWhitespace();
        if (i < content.size() && content[i] == ',') i++; // تخطي الفاصلة
    }

    return !mStrings.empty();
}

} // namespace ui
