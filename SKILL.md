---
name: duckh-dev
description: assist with development tasks for the duckh game project built with c++20, vulkan, and glsl. use when the user asks to add a feature, fix a bug, debug build errors, adjust rendering, change gameplay, update text rendering, modify shaders, or refactor code inside duckh. prefer reading the smallest relevant set of files first, follow the project map in references/duckh-map.md, preserve current behavior unless the user requests a change, and keep the project buildable with cmake --build build.
---

# duckh dev

Use this skill as an execution guide for work inside the DuckH codebase.

## Core operating rules

- Read `references/duckh-map.md` before exploring the codebase.
- Treat `references/duckh-map.md` as the source of truth for where logic lives.
- Read the fewest files needed to solve the request.
- Do not scan the whole project unless the task genuinely spans multiple systems.
- Preserve current behavior unless the user explicitly wants behavior changed.
- Prefer minimal, local edits over wide refactors.
- If a task would require risky structural changes, say so and propose the safer path.

## Workflow

1. Classify the request into one or more areas:
   - hunter / duck gameplay
   - rendering / vulkan / shaders
   - environment / atmosphere
   - animation / sprite atlases
   - audio
   - input / window / timing
   - text rendering / arabic / bidi / hud text
   - build or project structure
2. Open `references/duckh-map.md` and identify the narrowest matching files.
3. Read only those files and directly related headers or implementation files.
4. Make the smallest safe edit that satisfies the request.
5. If files are added or project structure changes, also update `SKILL.md` and `quide.md` in the project.
6. When the task touches build-sensitive code, recommend or run `cmake --build build` if appropriate.
7. End with a compact summary:
   - what changed
   - files changed
   - whether build verification is recommended

## Reading rules

- For gameplay requests, start in `src/gameplay/GameplayManager.cpp` and the helper files named in the map.
- For rendering requests, start in `src/rendering/RenderManager.cpp`, `src/rendering/SceneLayout.*`, `src/gfx/RenderTypes.h`, and `src/gfx/VulkanContext.*` as needed.
- For text requests, do not assume legacy localization files exist. Use the `src/text/*` subsystem and `src/ui/TextAtlas.*` as described in the reference map.
- For audio requests, start with `src/audio/SoundEffectPlayer.*`.
- For input and app lifecycle requests, use `src/core/Input.*`, `src/core/Window.*`, `src/core/Timer.*`, and `src/core/App.*` selectively.
- For shader requests, inspect only the specific files under `assets/shaders/vert/*` or `assets/shaders/frag/*` that match the effect.

## Project conventions

- Language: `C++20`.
- Use `#pragma once` in headers.
- Write explanatory comments in Arabic.
- Keep file names and class names in English.
- Use `camelCase` for functions and variables.
- Do not dump new gameplay logic into `App.cpp` if it belongs in `AppGameplay.cpp` or `AppRender.cpp`.
- Avoid modifying third-party files unless the task is specifically about integrating or upgrading them.

## Output style

Use a practical, execution-first style.

When making changes, briefly state:
- what you understood
- which files you inspected
- what you changed
- any build follow-up needed

Avoid long theory unless the user asks for it.
