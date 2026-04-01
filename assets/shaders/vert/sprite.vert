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

// Constants for fine-tuning
const float kWindPower = 1.0;
const float kFootPush = 1.0;

void main() {
    vec2 pos = inPosition;
    float localHeight = 1.0 - inTexCoord.y;
    float flex = inWindWeight * smoothstep(0.2, 0.95, localHeight);

    // 1. حساب تفاعل الأقدام (Radial Interaction)
    float footA = (1.0 - smoothstep(ubo.interactionA.y, ubo.interactionA.y + 0.08, abs(pos.x - ubo.interactionA.x))) * ubo.interactionA.z;
    float footB = (1.0 - smoothstep(ubo.interactionB.y, ubo.interactionB.y + 0.08, abs(pos.x - ubo.interactionB.x))) * ubo.interactionB.z;
    float footBlend = max(footA, footB) * kFootPush;

    // 2. قراءة خريطة الرياح (Wind Field Sampling)
    vec2 windUv = vec2(pos.x * 0.4 + ubo.motion.x * ubo.motion.z * 0.03, localHeight * 0.2);
    vec3 windSample = texture(windMap, windUv).rgb * 2.0 - 1.0;

    // 3. دمج الرياح مع ضغط الأقدام
    float windAmp = ubo.motion.y * kWindPower * inWindResponse * flex * (1.0 - footBlend * 0.85);
    pos.x += windSample.x * windAmp;
    pos.y += (abs(windSample.y) * ubo.wind.y * ubo.motion.w * windAmp) + (footBlend * flex * 0.12);

    gl_Position = vec4(pos, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragAlpha = inAlpha;
    fragWindPhase = inWindPhase;
    fragMaterialType = inMaterialType;
    fragScreenX = pos.x;
}
