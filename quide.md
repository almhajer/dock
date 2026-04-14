# quide.md

## شجرة المشروع

```text
dock/
├── AGENT.md                                                # تعليمات تشغيلية داخلية لطريقة العمل
├── CMakeLists.txt                                          # إعداد البناء وربط GLFW و Vulkan وتجهيز حزمة macOS
├── SKILL.md                                                # توجيهات مهارية إضافية للمشروع أو الوكيل
├── quide.md                                                # هذا الدليل الشجري لملفات المشروع
│
├── assets/                                                 # كل الأصول المستخدمة وقت التشغيل
│   ├── audio/                                              # ملفات الصوت
│   │   ├── douck.wav                                       # صوت ظهور/تحليق البطة ويتوقف عند غيابها أو إصابتها
│   │   └── hunter_shot.mp3                                 # صوت إطلاق النار الخاص بالصياد
│   │
│   ├── macos/                                              # ملفات أيقونات وحزمة macOS
│   │   ├── DuckHunterStarter.icns                          # أيقونة التطبيق النهائية على macOS
│   │   ├── DuckHunterStarter.png                           # نسخة PNG مرجعية للأيقونة
│   │   └── DuckHunterStarter.iconset/                      # أحجام الأيقونة الخام قبل توليد icns
│   │       ├── icon_16x16.png                              # أيقونة صغيرة جداً
│   │       ├── icon_16x16@2x.png                           # نسخة Retina للحجم 16
│   │       ├── icon_32x32.png                              # أيقونة صغيرة
│   │       ├── icon_32x32@2x.png                           # نسخة Retina للحجم 32
│   │       ├── icon_128x128.png                            # أيقونة متوسطة
│   │       ├── icon_128x128@2x.png                         # نسخة Retina للحجم 128
│   │       ├── icon_256x256.png                            # أيقونة كبيرة
│   │       ├── icon_256x256@2x.png                         # نسخة Retina للحجم 256
│   │       ├── icon_512x512.png                            # أيقونة كبيرة جداً
│   │       └── icon_512x512@2x.png                         # أعلى دقة خام للأيقونة
│   │
│   ├── shaders/                                            # شيدرات الرسم
│   │   ├── frag/                                           # شيدرات مجزأة Fragment
│   │   │   ├── environment_cloud.frag                      # تلوين الغيوم
│   │   │   ├── environment_cloud.frag.spv                  # النسخة المترجمة لشيدر الغيوم
│   │   │   ├── environment_instanced.frag                  # تلوين عناصر البيئة المكررة
│   │   │   ├── environment_instanced.frag.spv              # النسخة المترجمة لشيدر البيئة المكررة
│   │   │   ├── environment_tree.frag                       # تلوين الأشجار
│   │   │   ├── environment_tree.frag.spv                   # النسخة المترجمة لشيدر الأشجار
│   │   │   ├── god_rays.frag                               # أشعة الشمس والضوء الجوي
│   │   │   ├── god_rays.frag.spv                           # النسخة المترجمة لشيدر أشعة الشمس
│   │   │   ├── sprite.frag                                 # تلوين السبرايت النهائي
│   │   │   ├── sprite.frag.spv                             # النسخة المترجمة لشيدر السبرايت
│   │   │   ├── sun_disk.frag                               # رسم قرص الشمس والهالة
│   │   │   └── sun_disk.frag.spv                           # النسخة المترجمة لشيدر الشمس
│   │   │
│   │   └── vert/                                           # شيدرات رأسية Vertex
│   │       ├── environment_cloud.vert                      # تموضع الغيوم
│   │       ├── environment_cloud.vert.spv                  # النسخة المترجمة لشيدر الغيوم الرأسي
│   │       ├── environment_instanced.vert                  # تموضع عناصر البيئة المكررة
│   │       ├── environment_instanced.vert.spv              # النسخة المترجمة لشيدر البيئة الرأسي
│   │       ├── environment_tree.vert                       # تموضع الأشجار
│   │       ├── environment_tree.vert.spv                   # النسخة المترجمة لشيدر الأشجار الرأسي
│   │       ├── fullscreen_triangle.vert                    # قاعدة رسم تأثيرات الشاشة الكاملة
│   │       ├── fullscreen_triangle.vert.spv                # النسخة المترجمة لشيدر الشاشة الكاملة
│   │       ├── sprite.vert                                 # تحويل رؤوس السبرايت
│   │       └── sprite.vert.spv                             # النسخة المترجمة لشيدر السبرايت الرأسي
│   │
│   └── sprite/                                             # صور السبرايت الرئيسية
│       ├── duck.png                                        # المصدر الوحيد لاستخراج فريمات البطة
│       └── hunter_atlas.png                                # أطلس الصياد: مشي + إطلاق أمامي + إطلاق عالي
│
├── macos/                                                  # ملفات مساعدة لحزمة التطبيق
│   └── Info.plist.in                                       # قالب Info.plist لتطبيق macOS
│
├── examples/                                               # أمثلة مرجعية لا تدخل في بناء اللعبة
│   └── text/                                               # أمثلة استعمال وتطوير Text subsystem
│       ├── README.md                                       # شرح سريع للأمثلة والسلاسل المقترحة للتجربة
│       ├── TextSystemQuickStart.cpp                        # مثال minimal من UTF-8 حتى vertices جاهزة للرندر
│       └── VulkanHudIntegrationExample.cpp                 # مثال دمج مع HUD ورفع vertices إلى ذاكرة Vulkan
│
└── src/                                                    # الكود المصدري الكامل
    ├── main.cpp                                            # نقطة دخول البرنامج وإنشاء App
    │
    ├── audio/                                              # طبقة الصوت
    │   ├── SoundEffectPlayer.cpp                           # تنفيذ مشغل التأثيرات: تحميل/تشغيل/حلقة/إيقاف
    │   ├── SoundEffectPlayer.h                             # واجهة مشغل المؤثرات الصوتية
    │   ├── miniaudio_impl.cpp                              # ملف تفعيل تنفيذ مكتبة miniaudio
    │   └── third_party/
    │       └── miniaudio.h                                 # المكتبة الخارجية الخاصة بالصوت
    │
    ├── core/                                               # قلب التطبيق وتنظيم اللعب
    │   ├── App.cpp                                         # دورة الحياة: init / run / cleanup
    │   ├── App.h                                           # تعريف حالة التطبيق العامة ومكوناته
    │   ├── AppGameplay.cpp                                 # منطق اللعبة: الصياد، البطة، الإصابة، إعادة الرحلة
    │   ├── AppRender.cpp                                   # تجهيز طبقات الرسم: الأرض، العشب، الصياد، البطة
    │   ├── DuckGameplay.cpp                                # تنفيذ مساعدات ومنحنيات حركة البطة
    │   ├── DuckGameplay.h                                  # ثوابت البطة ودوالها المساعدة
    │   ├── HunterGameplay.cpp                              # تنفيذ مساعدات الصياد: المؤشر، الاتجاه، تثبيت الإطلاق
    │   ├── HunterGameplay.h                                # واجهة منطق الصياد المساعد
    │   ├── Input.cpp                                       # تنفيذ التقاط إدخال المستخدم من GLFW
    │   ├── Input.h                                         # تعريف واجهة الإدخال
    │   ├── SceneLayout.cpp                                 # حساب تموضع العناصر على الشاشة وآثار الأقدام
    │   ├── SceneLayout.h                                   # تعريف قياسات الشاشة وبناء quads للعناصر
    │   ├── Timer.cpp                                       # حساب الزمن بين الإطارات
    │   ├── Timer.h                                         # تعريف المؤقت
    │   ├── Window.cpp                                      # إنشاء النافذة والتحقق من جلسة GUI على macOS
    │   └── Window.h                                        # تعريف واجهة النافذة
    │
    ├── game/                                               # منطق بيانات اللعبة والأنيميشن والأطالس
    │   ├── DuckSpriteAtlas.cpp                             # بناء أطلس البطة واستخراج 11 فريم من duck.png
    │   ├── DuckSpriteAtlas.h                               # واجهة أطلس البطة
    │   ├── HunterSpriteAtlas.cpp                           # توصيف فريمات الصياد ومقاطع الحركة والإطلاق
    │   ├── HunterSpriteAtlas.h                             # واجهة أطلس الصياد
    │   ├── HunterSpriteData.cpp                            # قيم توقيتات الصياد مثل الانتظار بعد الإطلاق
    │   ├── HunterSpriteData.h                              # تعريف توقيتات الصياد
    │   ├── NatureSystem.cpp                                # توليد وتحديث الغيوم والأشجار وبيانات البيئة
    │   ├── NatureSystem.h                                  # واجهة نظام البيئة
    │   ├── SpriteAnimation.cpp                             # تشغيل المقاطع وتحديث الفريمات الحالية
    │   ├── SpriteAnimation.h                               # واجهة نظام الأنيميشن
    │   └── SpriteAtlas.h                                   # هياكل بيانات الفريمات والكليبات
    │
    ├── gfx/                                                # طبقة الرسوميات وVulkan
    │   ├── AtmosphereRenderer.cpp                          # تنفيذ مؤثرات الجو والشمس والأشعة
    │   ├── AtmosphereRenderer.h                            # تعريف واجهة ومخازن الغلاف الجوي
    │   ├── EnvironmentAtlas.cpp                            # توليد خامات البيئة داخل الذاكرة
    │   ├── EnvironmentAtlas.h                              # واجهة أطلس البيئة
    │   ├── EnvironmentTypes.h                              # تعريف أنواع بيانات البيئة
    │   ├── RenderTypes.h                                   # تعريف TexturedQuad والرؤوس والشفافية والدوران
    │   ├── VulkanContext.cpp                               # تنفيذ محرك Vulkan والطبقات والخامات والرسم
    │   ├── VulkanContext.h                                 # واجهة VulkanContext
    │   └── stb_image.h                                     # مكتبة تحميل الصور
    │
    ├── text/                                               # Text subsystem مستقل للتحليل والتشكيل والـ layout
    │   ├── README.md                                       # شرح معماري سريع ونقاط الدمج والاستبدال المستقبلي
    │   ├── TextTypes.h                                     # الأنواع والعقود الأساسية بين BiDi وShaper وLayout وRender bridge
    │   ├── Utf8.cpp                                        # فك وترميز UTF-8 بشكل آمن مع fallback إلى U+FFFD
    │   ├── Utf8.h                                          # واجهة UTF-8 decoding/encoding
    │   ├── ArabicJoiner.cpp                                # خصائص الاتصال وجداول presentation forms للحروف العربية الشائعة
    │   ├── ArabicJoiner.h                                  # واجهة تصنيف الأحرف العربية والوصل
    │   ├── BasicBidiResolver.cpp                           # ترتيب بصري مبسط للـ runs المختلطة عربي/لاتيني/أرقام
    │   ├── BasicBidiResolver.h                             # واجهة محلل BiDi الداخلي
    │   ├── BasicArabicShaper.cpp                           # تشكيل العربية إلى isolated/initial/medial/final مع lam-alef
    │   ├── BasicArabicShaper.h                             # واجهة الـ shaper العربي الداخلي
    │   ├── InternalGlyphProvider.cpp                       # قياسات وUV placeholders مستقلة عن أي مكتبة خطوط خارجية
    │   ├── InternalGlyphProvider.h                         # واجهة المزود الداخلي لقياسات glyphs
    │   ├── TextLayouter.cpp                                # دمج decoding وBiDi وshaping وقياسات glyphs في TextLayout واحد
    │   ├── TextLayouter.h                                  # واجهة الـ layouter وخيارات التخطيط والمحاذاة الأفقية
    │   ├── TextRendererTypes.h                             # أنواع vertices/quads المستقلة عن Vulkan
    │   ├── TextRendererBridge.cpp                          # تحويل TextLayout إلى GlyphQuads وflat vertices قابلة للرفع إلى buffers
    │   ├── TextRendererBridge.h                            # واجهة الجسر بين الـ layout والرندرر الحالي
    │   ├── TextSystem.cpp                                  # Facade موحدة من النص الخام حتى quads/vertices الجاهزة
    │   └── TextSystem.h                                    # نقطة الدخول الأعلى مستوى لواجهات اللعبة السريعة
    │
    └── ui/                                                 # طبقة النصوص وواجهة المستخدم المبنية فوق الأطلس
        ├── TextAtlas.cpp                                   # توليد أطلس النصوص وقت التشغيل مع دعم العربية عبر CoreText
        └── TextAtlas.h                                     # واجهة قياسات الرموز وUV والرسم النصي
```

## ملاحظات سريعة

- ملفات `src/core` أصبحت مقسمة حسب المسؤولية:
  - `App.cpp` لدورة الحياة
  - `AppGameplay.cpp` لمنطق اللعب
  - `AppRender.cpp` للرسم
- منطق الصياد والبطة المساعد جرى فصله في:
  - `HunterGameplay.h/.cpp`
  - `DuckGameplay.h/.cpp`
- هذا الملف يعرض المشروع كشجرة فقط، مع وصف مختصر ملاصق لكل ملف.
