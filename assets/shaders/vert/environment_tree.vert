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
layout(location = 3) flat out vec4 treeParams;
layout(location = 4) flat out float windBlend;

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
    float bendMask = smoothstep(0.14, 0.98, localHeight);
    bendMask *= bendMask;
    float tipMask = smoothstep(0.48, 1.0, localHeight);

    // جذع الشجرة ثابت تقريبًا، بينما تتفاعل القمم والأطراف مع الريح.
    vec2 windUv = vec2(
        inCenter.x * 0.16 + time * 0.010,
        inWindData.y * 0.051 + localHeight * 0.22
    );
    vec2 windSample = textureLod(windMap, windUv, 0.0).rg * 2.0 - 1.0;

    float macro =
        sin(time * 0.34 + inCenter.x * 1.40 + inWindData.y) +
        0.28 * sin(time * 0.18 - inCenter.x * 0.72 + inWindData.y * 1.6);
    float micro = windSample.x * 0.22 + sin(time * 0.90 + inWindData.y * 2.0 + localHeight * 4.8) * 0.05;

    float sway = (macro * 0.72 + micro) * inShapeData.z * inWindData.x * inWindData.z * bendMask;
    float edgeFlutter = windSample.y * 0.010 * tipMask * abs(inQuadPos.x);

    vec2 pos = inCenter + inQuadPos * inHalfSize;
    pos.x += sway + edgeFlutter * sign(inQuadPos.x);
    pos.y += abs(sway) * 0.10 * tipMask;

    gl_Position = vec4(pos, 0.0, 1.0);

    atlasUv = mix(inUvRect.xy, inUvRect.zw, inQuadUv);
    localUv = inQuadUv;
    tintAlpha = inTintAlpha;
    treeParams = vec4(
        hash12(inCenter + vec2(inWindData.y, inShapeData.z)),
        inShapeData.y,
        inShapeData.z,
        inWindData.w
    );
    windBlend = saturate(abs(sway) * 14.0 + abs(edgeFlutter) * 48.0);
}
