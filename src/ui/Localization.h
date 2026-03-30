#pragma once

#include <string>
#include <unordered_map>
#include <string_view>

namespace ui {

/// نظام الترجمة - يحمّل النصوص من ملفات JSON ويدعم العربية والإنجليزية
class Localization {
public:
    /// اللغات المدعومة
    enum class Language {
        Arabic,
        English
    };

    Localization() = default;

    /// تحميل ملف اللغة من مسار الأصول
    /// @param assetsPath مسار مجلد assets
    /// @param lang اللغة المطلوبة
    /// @return true عند النجاح
    bool load(const std::string& assetsPath, Language lang);

    /// الحصول على نص مترجم بالمفتاح
    /// @param key مفتاح النص مثل "menu.title"
    /// @return النص المترجم أو المفتاح نفسه إذا لم يُوجد
    [[nodiscard]] const std::string& get(const std::string& key) const;

    /// الحصول على نص مع استبدال {n} بقيمة رقمية
    /// @param key مفتاح النص
    /// @param value القيمة المراد إدراجها مكان {n}
    /// @return النص مع القيمة المُدرجة
    [[nodiscard]] std::string getFormatted(const std::string& key, int value) const;

    /// تغيير اللغة الحالية (يعيد تحميل الملف)
    void setLanguage(const std::string& assetsPath, Language lang);

    /// اللغة الحالية
    [[nodiscard]] Language getCurrentLanguage() const;

    /// اسم اللغة كنص ("ar" أو "en")
    [[nodiscard]] std::string_view getLanguageCode() const;

    /// هل اللغة الحالية RTL (عربي)؟
    [[nodiscard]] bool isRTL() const;

private:
    /// تحليل ملف JSON يدويًا (بدون مكتبة خارجية)
    bool parseJSON(const std::string& content);

    std::unordered_map<std::string, std::string> mStrings;
    Language mCurrentLang = Language::English;
};

} // namespace ui
