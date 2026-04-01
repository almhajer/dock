#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in float inAlpha;
layout(location = 3) in float inWindWeight;
layout(location = 4) in float inWindPhase;
layout(location = 5) in float inWindResponse;
layout(location = 6) in float inMaterialType;

layout(binding = 1) uniform WindUniformBuffer {
    vec4 motion;       // x=time, y=strength, z=speed, w=sag
    vec4 wind;         // x=direction, y=v-scale
    vec4 interactionA; // x=pos, y=radius, z=pressure
    vec4 interactionB;
} ubo;

layout(binding = 2) uniform sampler2D windMap;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out float fragAlpha;
layout(location = 2) out float fragWindPhase;
layout(location = 3) out float fragMaterialType;
layout(location = 4) out float fragScreenX;

// --- Per-instance hash ---
float hash11(float p) {
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

// --- 2D hash & noise for micro wind ---
float hash2(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float noise2D(vec2 uv) {
    vec2 i = floor(uv);
    vec2 f = fract(uv);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash2(i);
    float b = hash2(i + vec2(1.0, 0.0));
    float c = hash2(i + vec2(0.0, 1.0));
    float d = hash2(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

void main() {
    vec2 pos = inPosition;
    float t       = ubo.motion.x;
    float str     = ubo.motion.y;
    float spd     = ubo.motion.z;
    float sag     = ubo.motion.w;

    float localHeight = 1.0 - inTexCoord.y;

    // --- Per-instance randomization ---
    float rndPhase  = hash11(inWindPhase + 3.7) * 6.2832;
    float rndLean   = (hash11(inWindPhase + 7.3) - 0.5) * 0.015;
    float hFactor   = mix(0.7, 1.3, hash11(inWindPhase + 11.1));
    float rndMicro  = hash11(inWindPhase + 15.9);

    // --- Nonlinear height-based bending (quadratic) ---
    float flex = inWindWeight * smoothstep(0.15, 0.95, localHeight);
    flex *= flex;

    // Taller blades respond more
    float response = inWindResponse * hFactor;

    // === MACRO WIND: slow large-scale sinusoidal sweep ===
    float macroP = t * 0.25 * spd + pos.x * 1.2 + rndPhase;
    float macroWind = sin(macroP) * 0.55
                    + sin(macroP * 0.6 + 1.8) * 0.30
                    + sin(macroP * 1.4 + 3.1) * 0.15;
    macroWind *= str * flex * response;

    // === MICRO WIND: wind field texture + procedural noise ===
    vec2 windUv = vec2(pos.x * 0.4 + t * spd * 0.03, localHeight * 0.2);
    vec3 windSample = texture(windMap, windUv).rgb * 2.0 - 1.0;

    float microN = noise2D(vec2(pos.x * 4.0 + t * spd * 0.45, t * 0.25 + rndMicro)) * 0.25
                 + noise2D(vec2(pos.x * 9.0 - t * spd * 0.6, t * 0.4 - rndMicro * 0.5)) * 0.12;

    float microWind = (windSample.x * 0.7 + microN) * str * flex * response;

    // === FOOT INTERACTION ===
    float footA = (1.0 - smoothstep(ubo.interactionA.y, ubo.interactionA.y + 0.08,
                                     abs(pos.x - ubo.interactionA.x))) * ubo.interactionA.z;
    float footB = (1.0 - smoothstep(ubo.interactionB.y, ubo.interactionB.y + 0.08,
                                     abs(pos.x - ubo.interactionB.x))) * ubo.interactionB.z;
    float footBlend = max(footA, footB);

    // === COMBINE ===
    float dampen = 1.0 - footBlend * 0.85;
    pos.x += (macroWind + microWind + rndLean) * dampen;
    pos.y += abs(windSample.y) * sag * str * flex * response * dampen
           + footBlend * flex * 0.12;

    gl_Position = vec4(pos, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragAlpha = inAlpha;
    fragWindPhase = inWindPhase;
    fragMaterialType = inMaterialType;
    fragScreenX = pos.x;
}
