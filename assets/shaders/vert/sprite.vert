#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in float inAlpha;
layout(location = 3) in float inWindWeight;
layout(location = 4) in float inWindPhase;
layout(location = 5) in float inWindResponse;
layout(location = 6) in float inMaterialType;

layout(binding = 1) uniform WindUniformBuffer {
    vec4 motion;
    vec4 wind;
    vec4 interactionA;
    vec4 interactionB;
} ubo;

layout(binding = 2) uniform sampler2D windMap;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out float fragAlpha;
layout(location = 2) out float fragWindPhase;
layout(location = 3) out float fragMaterialType;
layout(location = 4) out vec2 fragFieldCoord;

float hash11(float p) {
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

void main() {
    vec2 basePos = inPosition;
    vec2 pos = basePos;
    float time = ubo.motion.x;
    float strength = ubo.motion.y;
    float speed = ubo.motion.z;
    float sag = ubo.motion.w;

    float localHeight = 1.0 - inTexCoord.y;
    float bendMask = smoothstep(0.10, 0.96, localHeight);
    bendMask *= bendMask;
    float tipMask = smoothstep(0.50, 1.0, localHeight);

    float phaseSeed = inWindPhase;
    float phase = hash11(phaseSeed + 2.37) * 6.2831853;
    float baseLean = (hash11(phaseSeed + 5.91) - 0.5) * 0.018;
    float stiffness = mix(0.86, 1.14, hash11(phaseSeed + 10.4));
    float flexibility = mix(0.82, 1.20, hash11(phaseSeed + 14.2));
    float response = inWindResponse * flexibility / stiffness;

    float macroWind = sin(time * 0.24 * speed + basePos.x * 1.28 + phase);
    macroWind += 0.26 * sin(time * 0.13 * speed - basePos.x * 0.54 + phase * 1.4);
    macroWind *= 0.74;

    vec2 windUv = vec2(
        basePos.x * 0.24 + time * speed * 0.012,
        phaseSeed * 0.041 + localHeight * 0.10);
    vec3 windSample = texture(windMap, windUv).rgb * 2.0 - 1.0;

    float microWind =
        windSample.x * 0.34 +
        sin(time * 0.95 * speed + phase * 2.2 + localHeight * 5.0) * 0.08;

    float horizontalOffset =
        (macroWind + microWind) * strength * response * bendMask +
        baseLean * bendMask;

    float footA =
        (1.0 - smoothstep(ubo.interactionA.y, ubo.interactionA.y + 0.10,
                          abs(basePos.x - ubo.interactionA.x))) * ubo.interactionA.z;
    float footB =
        (1.0 - smoothstep(ubo.interactionB.y, ubo.interactionB.y + 0.10,
                          abs(basePos.x - ubo.interactionB.x))) * ubo.interactionB.z;
    float footBlend = max(footA, footB) * smoothstep(0.14, 0.96, localHeight);
    float dampen = 1.0 - footBlend * 0.78;

    pos.x += horizontalOffset * dampen;
    pos.y +=
        (abs(horizontalOffset) * 0.62 + abs(windSample.y) * strength * 0.14) *
        sag * response * mix(0.22, 1.0, tipMask) * dampen +
        footBlend * bendMask * 0.11;

    gl_Position = vec4(pos, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragAlpha = inAlpha;
    fragWindPhase = inWindPhase;
    fragMaterialType = inMaterialType;
    fragFieldCoord = basePos;
}
