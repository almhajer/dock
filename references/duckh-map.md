# DuckH project map

Use this file before reading the repository. It narrows file selection so work can start quickly without scanning the whole codebase.

## Build and runtime

- Build: `cmake --build build`
- Run on macOS: `open build/DuckH.app`
- Run from terminal: `./build/DuckH.app/Contents/MacOS/DuckH`
- There are currently no automated tests.

## Stable project rules

- Language: `C++20`
- Use `#pragma once` in headers
- Comments and explanatory notes should be in Arabic
- File names and class names stay in English
- Function and variable names use `camelCase`
- If a new file is added or project structure changes, update the project's `SKILL.md` and `quide.md`

## Fast routing by task

### Hunter tasks

Use these first for hunter movement, shooting, aiming direction, and shot frame timing:
- `src/gameplay/GameplayManager.cpp`
- `src/gameplay/HunterController.h`
- `src/gameplay/HunterController.cpp`

Use these for hunter atlas frames and animation clips:
- `src/assets/HunterSpriteAtlas.cpp`
- `src/assets/HunterSpriteAtlas.h`

Use these for hunter timing data such as reload timing and post-shot hold:
- `src/assets/HunterSpriteData.cpp`
- `src/assets/HunterSpriteData.h`

### Duck tasks

Use these first for duck spawn, flight, hit, fall, and despawn logic:
- `src/gameplay/GameplayManager.cpp`

Use these for duck movement constants, bezier paths, and hit-frame hold:
- `src/gameplay/DuckGameplay.h`
- `src/gameplay/DuckGameplay.cpp`

Use these for building duck atlas data from `duck.png`:
- `src/assets/DuckSpriteAtlas.cpp`
- `src/assets/DuckSpriteAtlas.h`

### Rendering tasks

Use these first for draw layers, hunter and duck rendering, transparency, and ground or grass updates:
- `src/rendering/RenderManager.cpp`

Use these for element placement and screen-space layout:
- `src/rendering/SceneLayout.cpp`
- `src/rendering/SceneLayout.h`

Use these for render structs such as textured quads, rotation, and alpha:
- `src/gfx/RenderTypes.h`

Use these for Vulkan resource management, layering, and actual rendering:
- `src/gfx/VulkanContext.cpp`
- `src/gfx/VulkanContext.h`

### Environment tasks

Use these for clouds, trees, and moving environment data:
- `src/game/NatureSystem.cpp`
- `src/game/NatureSystem.h`

Use these for procedural environment textures or visual generation:
- `src/gfx/EnvironmentAtlas.cpp`
- `src/gfx/EnvironmentAtlas.h`

Use these for atmosphere, sun, and god rays:
- `src/gfx/AtmosphereRenderer.cpp`
- `src/gfx/AtmosphereRenderer.h`

### Animation and atlas tasks

Use these for animation playback and frame progression:
- `src/game/SpriteAnimation.cpp`
- `src/game/SpriteAnimation.h`

Use these for frame, clip, and atlas definitions:
- `src/game/SpriteAtlas.h`

### Audio tasks

Use these for loading, playing, stopping, and looping sound effects:
- `src/audio/SoundEffectPlayer.cpp`
- `src/audio/SoundEffectPlayer.h`

Use these for the audio engine integration itself:
- `src/audio/third_party/miniaudio.h`
- `src/audio/miniaudio_impl.cpp`

### Input, window, and timing tasks

Use these for keyboard and mouse handling:
- `src/core/Input.cpp`
- `src/core/Input.h`

Use these for window creation and macOS graphics-session checks:
- `src/core/Window.cpp`
- `src/core/Window.h`

Use these for frame timing:
- `src/core/Timer.cpp`
- `src/core/Timer.h`

### Text and hud text tasks

Use the new text subsystem first:
- `src/text/TextTypes.h`
- `src/text/Utf8.h`
- `src/text/Utf8.cpp`
- `src/text/ArabicJoiner.h`
- `src/text/ArabicJoiner.cpp`
- `src/text/BasicBidiResolver.h`
- `src/text/BasicBidiResolver.cpp`
- `src/text/BasicArabicShaper.h`
- `src/text/BasicArabicShaper.cpp`
- `src/text/InternalGlyphProvider.h`
- `src/text/InternalGlyphProvider.cpp`
- `src/text/TextLayouter.h`
- `src/text/TextLayouter.cpp`
- `src/text/TextRendererTypes.h`
- `src/text/TextRendererBridge.h`
- `src/text/TextRendererBridge.cpp`
- `src/text/TextSystem.h`
- `src/text/TextSystem.cpp`
- `src/text/README.md`

Use these for bitmap text atlas generation and the current UI text atlas path:
- `src/ui/TextAtlas.cpp`
- `src/ui/TextAtlas.h`

Important:
- Do not assume `Localization.cpp/.h` exists.
- Do not assume `assets/lang/*.json` is the active text path.
- `src/text/*` is the active analysis, shaping, bidi, layout, and render-prep path.
- `src/text/TextSystem.*` is the fastest entry point for HUD text integration.
- `src/ui/TextAtlas.*` remains a separate bitmap-atlas path.
- Examples live in:
  - `examples/text/TextSystemQuickStart.cpp`
  - `examples/text/VulkanHudIntegrationExample.cpp`
  - `examples/text/README.md`

### Assets and shaders

Use these when the request is about assets or visual inputs:
- Duck sprite: `assets/sprite/duck.png`
- Hunter atlas: `assets/sprite/hunter_atlas.png`
- Duck sound: `assets/audio/douck.wav`
- Shot sound: `assets/audio/hunter_shot.mp3`
- Vertex shaders: `assets/shaders/vert/*`
- Fragment shaders: `assets/shaders/frag/*`

## Usage notes

1. Identify the task type first.
2. Start from the matching section above.
3. Read only the listed files and any directly required counterparts.
4. Do not widen the search unless the edit truly spans multiple systems.
