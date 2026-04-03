#version 450

layout(location = 0) in vec2 inQuadPos;
layout(location = 1) in vec2 inQuadUv;
layout(location = 2) in vec2 inCenter;
layout(location = 3) in vec2 inHalfSize;
layout(location = 4) in vec4 inUvRect;
layout(location = 5) in vec4 inTintAlpha;
layout(location = 6) in vec4 inWindData;   // x = weight, y = phase, z = response, w = parallax
layout(location = 7) in vec4 inShapeData;  // x = kind, y = softness, z = swayAmount, w = reserved

layout(binding = 1) uniform WindUniformBuffer {
    vec4 motion;
    vec4 wind;
    vec4 interactionA;
    vec4 interactionB;
} ubo;

layout(binding = 2) uniform sampler2D windMap;

layout(location = 0) out vec2 atlasUv;
layout(location = 1) out vec2 localUv;
layout(location = 2) flat out vec4 tintAlpha;
layout(location = 3) flat out vec2 fieldCoord;
layout(location = 4) flat out vec2 cloudShape;
layout(location = 5) flat out float windBlend;

float saturate(float value)
{
    return clamp(value, 0.0, 1.0);
}

float hash12(vec2 p)
{
    vec3 q = fract(vec3(p.xyx) * 0.1031);
    q += dot(q, q.yzx + 33.33);
    return fract((q.x + q.y) * q.z);
}

void main()
{
    float time = ubo.motion.x * max(ubo.motion.z, 0.001);
    float localHeight = 1.0 - inQuadUv.y;
    float topMask = smoothstep(0.14, 1.0, localHeight);
    float sideMask = 1.0 - abs(inQuadUv.x * 2.0 - 1.0);

    // حركة السحابة تبقى في الـ vertex حتى لا يتحمل الـ fragment هذا التشوه.
    vec2 windUv = vec2(
        inCenter.x * 0.10 + time * 0.006,
        inWindData.y * 0.041 + localHeight * 0.18
    );
    vec2 windSample = textureLod(windMap, windUv, 0.0).rg * 2.0 - 1.0;

    float macro =
        sin(time * 0.08 + inWindData.y + inCenter.y * 8.0) +
        0.35 * sin(time * 0.03 - inCenter.x * 3.2 + inWindData.y * 1.7);

    float sway = (macro * 0.55 + windSample.x * 0.28) * inWindData.x * inWindData.z;
    float inflate = (windSample.y * 0.012 + macro * 0.004) * topMask * sideMask * inShapeData.y;

    vec2 pos = inCenter + inQuadPos * inHalfSize;
    pos.x += sway * (0.22 + topMask * 0.78);
    pos.y += sin(time * 0.05 + inWindData.y * 0.73 + inQuadUv.x * 2.6) * 0.004 * inWindData.z;
    pos += inQuadPos * vec2(inflate * 0.55, inflate * 0.18);

    gl_Position = vec4(pos, 0.0, 1.0);

    atlasUv = mix(inUvRect.xy, inUvRect.zw, inQuadUv);
    localUv = inQuadUv;
    tintAlpha = inTintAlpha;
    fieldCoord = inCenter;
    cloudShape = vec2(inShapeData.y, hash12(inCenter + vec2(inWindData.y, inWindData.w)));
    windBlend = saturate(abs(sway) * 10.0 + abs(inflate) * 42.0);
}
