# Text Subsystem

هذا المجلد يحتوي طبقة نصوص مستقلة عن Vulkan الخام ومقسمة إلى مراحل واضحة:

- `Utf8.*`
  - فك وترميز `UTF-8` بشكل آمن.
- `ArabicJoiner.*`
  - تصنيف الحروف العربية وخصائص الاتصال وخرائط `Presentation Forms`.
- `BasicBidiResolver.*`
  - تحليل مبسط لاتجاه السطر المختلط عربي/لاتيني/أرقام.
- `BasicArabicShaper.*`
  - تشكيل العربية إلى `isolated / initial / medial / final` مع `lam-alef`.
- `InternalGlyphProvider.*`
  - مزود قياسات placeholder مستقل عن أي rasterizer خارجي.
- `TextLayouter.*`
  - تحويل الـ runs المشكلة إلى `PositionedGlyph` مع دعم المحاذاة.
- `TextRendererBridge.*`
  - تجهيز `GlyphQuad` و`TextVertex` الجاهزين للرفع إلى `vertex buffer`.
- `TextSystem.*`
  - Facade عالية المستوى للاستخدام السريع داخل HUD أو UI.

## الاستخدام السريع

```cpp
#include "text/TextSystem.h"

textsys::TextSystem textSystem(18.0f);

textsys::TextLayoutOptions layoutOptions;
layoutOptions.originX = 0.92f;
layoutOptions.baselineY = 0.84f;
layoutOptions.fontScale = 1.0f;
layoutOptions.alignment = textsys::TextAlignment::End;

textsys::TextRenderStyle renderStyle;
renderStyle.color = {1.0f, 1.0f, 1.0f, 1.0f};

const textsys::PreparedText prepared = textSystem.prepareUtf8(
    u8"اضغط ENTER للفتح",
    {.layout = layoutOptions, .render = renderStyle});

// prepared.vertices جاهزة للنسخ إلى vertex buffer.
```

## لماذا هذا التنظيم

- Vulkan لا يعرف شيئًا عن `BiDi` أو `Arabic shaping`.
- تشكيل العربية لا يجب أن يعيش داخل `AppRender.cpp`.
- `TextSystem` يقلل wiring اليدوي، لكن الطبقات السفلية ما زالت مستقلة وقابلة للاستبدال.

## نقاط الاستبدال المستقبلية

- `BasicArabicShaper` -> `HarfBuzzShaper`
- `BasicBidiResolver` -> `IcuBidiResolver`
- `InternalGlyphProvider` -> `FreeTypeGlyphProvider`

العقود التي يجب الحفاظ عليها:

- `IBidiResolver`
- `ITextShaper`
- `IGlyphMetricsProvider`

## الملاحظات الحالية

- النظام مضبوط حاليًا لسطر واحد جيد لواجهات الألعاب.
- `BasicBidiResolver` approximation مقصودة وسريعة لحالات HUD الشائعة.
- `InternalGlyphProvider` لا يرسم glyph bitmap حقيقي؛ هو فقط يجهز المقاسات و`UV placeholders`.
