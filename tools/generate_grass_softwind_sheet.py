from __future__ import annotations

import json
import math
import random
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageDraw


FRAME_COUNT = 8
FRAME_WIDTH = 256
FRAME_HEIGHT = 192
SUPERSAMPLE = 4
GROUND_HEIGHT = 30

OUTPUT_IMAGE = Path("assets/sprite/grass_softwind_8f.png")
OUTPUT_META = Path("assets/sprite/grass_softwind_8f.json")


@dataclass(frozen=True)
class Blade:
    x: float
    height: float
    base_width: float
    lean: float
    response: float
    curl: float
    color: tuple[int, int, int, int]
    shadow: tuple[int, int, int, int]
    highlight: tuple[int, int, int, int]


def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def smoothstep(t: float) -> float:
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


def mix_color(c0: tuple[int, int, int], c1: tuple[int, int, int], t: float) -> tuple[int, int, int]:
    return (
        int(lerp(c0[0], c1[0], t)),
        int(lerp(c0[1], c1[1], t)),
        int(lerp(c0[2], c1[2], t)),
    )


def build_blades(rng: random.Random, width: int, height: int, ground_top: int) -> list[Blade]:
    blades: list[Blade] = []
    lanes = 88
    palette_low = (44, 109, 47)
    palette_high = (130, 206, 82)

    for lane in range(lanes):
        x = (lane + 0.5) * width / lanes + rng.uniform(-8.0, 8.0)
        x = max(8.0, min(width - 8.0, x))

        height_norm = rng.random()
        blade_height = lerp(height * 0.33, height * 0.72, height_norm)
        base_width = lerp(8.0, 20.0, height_norm) * rng.uniform(0.85, 1.12)
        response = lerp(0.48, 1.15, pow(height_norm, 1.35))
        lean = rng.uniform(-0.05, 0.05)
        curl = rng.uniform(-0.3, 0.3)

        shade_t = lerp(0.12, 0.92, height_norm * 0.85 + rng.uniform(-0.1, 0.1))
        base_rgb = mix_color(palette_low, palette_high, max(0.0, min(1.0, shade_t)))

        blades.append(
            Blade(
                x=x,
                height=blade_height,
                base_width=base_width,
                lean=lean,
                response=response,
                curl=curl,
                color=(*base_rgb, 255),
                shadow=(18, 54, 21, 120),
                highlight=(214, 255, 184, 92),
            )
        )

    blades.sort(key=lambda blade: blade.height)
    return blades


def blade_center(blade: Blade, bend: float, t: float, ground_top: float) -> tuple[float, float]:
    bend_profile = smoothstep((t - 0.18) / 0.82)
    sway = bend * blade.response
    x = (
        blade.x
        + blade.lean * blade.height * t * 0.17
        + sway * blade.height * 0.14 * bend_profile
        + blade.curl * 6.0 * math.sin(t * math.pi) * (1.0 - t * 0.45)
    )
    y = ground_top - blade.height * t
    return x, y


def blade_polygon(blade: Blade, bend: float, ground_top: float) -> tuple[list[tuple[float, float]], list[tuple[float, float]]]:
    samples = 22
    centers = [blade_center(blade, bend, i / (samples - 1), ground_top) for i in range(samples)]

    left_side: list[tuple[float, float]] = []
    right_side: list[tuple[float, float]] = []

    for i, (cx, cy) in enumerate(centers):
        if i == 0:
            px, py = centers[i + 1]
        elif i == samples - 1:
            px, py = centers[i - 1]
        else:
            px0, py0 = centers[i - 1]
            px1, py1 = centers[i + 1]
            px, py = (px1 - px0), (py1 - py0)

        dx = px - cx if i in (0, samples - 1) else px
        dy = py - cy if i in (0, samples - 1) else py
        length = math.hypot(dx, dy) or 1.0
        nx = -dy / length
        ny = dx / length

        t = i / (samples - 1)
        width = blade.base_width * pow(1.0 - t, 0.72) + 1.2
        if t > 0.84:
            width *= lerp(1.0, 0.22, (t - 0.84) / 0.16)

        half = width * 0.5
        left_side.append((cx - nx * half, cy - ny * half))
        right_side.append((cx + nx * half, cy + ny * half))

    polygon = left_side + list(reversed(right_side))
    return polygon, centers


def draw_ground(draw: ImageDraw.ImageDraw, frame_w: int, frame_h: int, ground_top: int) -> None:
    top = (65, 112, 48, 255)
    bottom = (42, 74, 31, 255)
    for y in range(ground_top, frame_h):
        t = (y - ground_top) / max(1, frame_h - ground_top - 1)
        color = (
            int(lerp(top[0], bottom[0], t)),
            int(lerp(top[1], bottom[1], t)),
            int(lerp(top[2], bottom[2], t)),
            255,
        )
        draw.line([(0, y), (frame_w, y)], fill=color)

    draw.rectangle((0, ground_top, frame_w, frame_h), outline=(26, 48, 20, 180), width=2)
    draw.rectangle((0, ground_top - 4, frame_w, ground_top + 2), fill=(77, 129, 55, 255))


def render_frame(blades: list[Blade], bend: float) -> Image.Image:
    scale = SUPERSAMPLE
    frame_w = FRAME_WIDTH * scale
    frame_h = FRAME_HEIGHT * scale
    ground_top = frame_h - GROUND_HEIGHT * scale

    frame = Image.new("RGBA", (frame_w, frame_h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(frame, "RGBA")

    for blade in blades:
        polygon, centers = blade_polygon(blade, bend, ground_top)
        draw.polygon(polygon, fill=blade.color)
        draw.line(polygon[: len(polygon) // 2], fill=blade.shadow, width=max(2, int(blade.base_width * 0.12)))
        draw.line(
            centers[3:-2],
            fill=blade.highlight,
            width=max(1, int(blade.base_width * 0.16)),
            joint="curve",
        )

    draw_ground(draw, frame_w, frame_h, ground_top)

    return frame.resize((FRAME_WIDTH, FRAME_HEIGHT), resample=Image.Resampling.LANCZOS)


def main() -> None:
    rng = random.Random(17)
    blades = build_blades(
        rng=rng,
        width=FRAME_WIDTH * SUPERSAMPLE,
        height=(FRAME_HEIGHT - GROUND_HEIGHT) * SUPERSAMPLE,
        ground_top=(FRAME_HEIGHT - GROUND_HEIGHT) * SUPERSAMPLE,
    )

    bends = [0.0, 0.09, 0.18, 0.26, 0.16, 0.05, -0.06, -0.02]

    sheet = Image.new("RGBA", (FRAME_WIDTH * FRAME_COUNT, FRAME_HEIGHT), (0, 0, 0, 0))
    for index, bend in enumerate(bends):
        frame = render_frame(blades, bend)
        sheet.paste(frame, (index * FRAME_WIDTH, 0), frame)

    OUTPUT_IMAGE.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(OUTPUT_IMAGE)

    metadata = {
        "image": OUTPUT_IMAGE.name,
        "frame_count": FRAME_COUNT,
        "layout": {
            "columns": FRAME_COUNT,
            "rows": 1,
            "frame_width": FRAME_WIDTH,
            "frame_height": FRAME_HEIGHT,
            "ground_height": GROUND_HEIGHT,
        },
        "frames": {
            f"frame_{i}": {
                "frame": {
                    "x": i * FRAME_WIDTH,
                    "y": 0,
                    "w": FRAME_WIDTH,
                    "h": FRAME_HEIGHT,
                }
            }
            for i in range(FRAME_COUNT)
        },
    }
    OUTPUT_META.write_text(json.dumps(metadata, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
