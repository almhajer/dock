# DuckH - لعبة صيد البط ثلاثية الأبعاد

## نظرة عامة
لعبة صيد بط ثلاثية الأبعاد مبنية بـ Vulkan + GLFW على macOS.
نمط اللعب: موجات متتالية بصعوبة متزايدة.
الواجهة: عربية + إنجليزية.

## الصلاحيات
- **Build & Compile**: cmake --build build
- **Run**: ./build/DuckH
- **Test**: لا توجد اختبارات حاليًا

## قواعد التطوير
- C++20 فقط، بدون امتدادات خاصة بالمترجم
- كل ملف مصدري `.cpp` يقابله ترويسة `.h` بنفس الاسم في نفس المجلد
- الترويسات التجميعية (مثل `Math.h`) بدون `.cpp`
- التعليقات والوثائق بالعربية
- أسماء الملفات والكلاسات بالإنجليزية (PascalCase)
- أسماء الدوال والمتغيرات بـ camelCase
- ثوابت الماكرو UPPER_SNAKE_CASE
- namespace لكل وحدة: `core::`, `gfx::`, `scene::`, `game::`, `ui::`, `audio::`, `util::`
- RAII دائمًا لإدارة موارد Vulkan
- تحقق من Vulkan 结果 بعد كل استدعاء vk*

## وحدات النظام

| الوحدة | المسار | الوصف |
|--------|--------|-------|
| النواة | `src/core/` | نافذة، إدخال، مؤقت، تطبيق |
| الرسوميات | `src/gfx/` | سياق Vulkan، أنابيب، كاميرا، شبكات |
| المشهد | `src/scene/` | كيانات، تحويلات، إضاءة، سماء |
| اللعبة | `src/game/` | منطق اللعبة، موجات، بط، لاعب |
| الواجهة | `src/ui/` | HUD، قوائم، ترجمة |
| الصوت | `src/audio/` | محرك صوت، مؤثرات |
| أدوات | `src/util/` | رياضيات، سجلات، عشوائي |

## مخطط الملفات

```
dock/
├── CMakeLists.txt                 # نظام البناء
├── CLAUDE.md                      # إعدادات Claude
├── SKILL.md                       # ← هذا الملف
├── AGENT.md                       # سير العمل والذكاء
│
├── assets/
│   ├── shaders/                   # ShadersSPIR-V
│   │   ├── vert/                  # Vertex shaders
│   │   └── frag/                  # Fragment shaders
│   ├── textures/                  # صور PNG/JPG
│   ├── models/                    # نماذج OBJ/GLB
│   ├── fonts/                     # خطوط TTF
│   ├── sounds/                    # أصوات WAV/OGG
│   └── lang/                      # ملفات الترجمة JSON
│       ├── ar.json
│       └── en.json
│
├── macos/
│   ├── Info.plist.in
│   └── DuckHunterStarter.icns
│
├── include/                       # ترويسات خارجية (فارغ حاليًا)
│
└── src/
    ├── main.cpp                   # نقطة الدخول
    │
    ├── core/                      # النواة
    │   ├── App.h / App.cpp        # حلقة التطبيق الرئيسية
    │   ├── Window.h / Window.cpp   # نافذة GLFW + أحداث
    │   ├── Input.h / Input.cpp     # إدارة لوحة المفاتيح والماوس
    │   └── Timer.h / Timer.cpp     # deltaTime + FPS
    │
    ├── gfx/                       # الرسوميات (Vulkan)
    │   ├── VulkanContext.h/cpp     # Device, Instance, Swapchain
    │   ├── Pipeline.h/cpp          # خط أنابيب الرسوميات
    │   ├── RenderPass.h/cpp        # تمريرات الرسم
    │   ├── Framebuffer.h/cpp       # Framebuffers
    │   ├── CommandBuffer.h/cpp     # أوامر الرسم
    │   ├── Buffer.h/cpp            # Vertex/Index/Uniform buffers
    │   ├── Image.h/cpp             # صور Vulkan + textures
    │   ├── Descriptor.h/cpp        # Descriptor sets/layouts
    │   ├── SyncObjects.h/cpp       # Semaphores + Fences
    │   ├── Camera.h/cpp            # كاميرا 3D (perspective)
    │   ├── Mesh.h/cpp              # بيانات الشبكة
    │   ├── Texture.h/cpp           # تحميل الصور
    │   └── Shader.h/cpp            # تحميل SPIR-V
    │
    ├── scene/                     # المشهد
    │   ├── Scene.h/cpp             # مدير المشهد
    │   ├── Entity.h/cpp            # كيان أساسي
    │   ├── Transform.h/cpp         # تحويلات 3D
    │   ├── Light.h/cpp             # إضاءة
    │   └── Skybox.h/cpp            # صندوق السماء
    │
    ├── game/                      # منطق اللعبة
    │   ├── Game.h/cpp              # مدير اللعبة + حالات
    │   ├── WaveManager.h/cpp       # نظام الموجات
    │   ├── Duck.h/cpp              # كيان البطة + حركتها
    │   ├── Player.h/cpp            # حالة اللاعب (نقاط، ذخيرة)
    │   ├── Bullet.h/cpp            # المقذوفات
    │   ├── Water.h/cpp             # سطح الماء
    │   ├── Environment.h/cpp       # أرض، أشجار، بحيرة
    │   └── Collision.h/cpp         # كشف التصادم
    │
    ├── ui/                        # الواجهة
    │   ├── UI.h/cpp                # مدير الواجهة
    │   ├── HUD.h/cpp               # شاشة اللعب (نقاط، ذخيرة، موجة)
    │   ├── Menu.h/cpp              # القائمة الرئيسية
    │   └── Localization.h/cpp      # ترجمة عربي/إنجليزي
    │
    ├── audio/                     # الصوت
    │   ├── Audio.h/cpp             # محرك الصوت
    │   └── Sound.h/cpp             # مورد صوتي
    │
    └── util/                      # أدوات
        ├── Math.h                  # Vec2, Vec3, Vec4, Mat4 (header-only)
        ├── Logger.h / Logger.cpp   # سجل الأخطاء والمعلومات
        └── Random.h                # أرقام عشوائية (header-only)
```

## ترتيب التنفيذ المقترح
1. `util/` ← أدوات أساسية (Math, Logger)
2. `core/` ← نافذة + إدخال + مؤقت
3. `gfx/` ← سياق Vulkan + مثلث تجريبي
4. `scene/` ← كيانات + كاميرا
5. `game/` ← منطق اللعبة + بط
6. `ui/` ← واجهة + ترجمة
7. `audio/` ← مؤثرات صوتية
8. تحسين + لمسات نهائية
