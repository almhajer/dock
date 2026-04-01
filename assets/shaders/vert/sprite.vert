#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in float inAlpha;
layout(location = 3) in float inWindWeight;
layout(location = 4) in float inWindPhase;
layout(location = 5) in float inWindResponse;
layout(location = 6) in float inMaterialType;

layout(binding = 1) uniform WindUniformBuffer {
    vec4 motion;       // x=time, y=wind power, z=wave speed, w=downward sag
    vec4 wind;         // x=horizontal direction, y=vertical response scale
    vec4 interactionA; // x=foot x, y=radius, z=pressure
    vec4 interactionB; // x=foot x, y=radius, z=pressure
} windUbo;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out float fragAlpha;
layout(location = 2) out float fragWindPhase;
layout(location = 3) out float fragMaterialType;
layout(location = 4) out float fragScreenX;

// Specialization constants تسمح بضبط الشيدر من Vulkan من دون إعادة ترجمته.
layout(constant_id = 0) const float kSpecWindStrength = 1.0;
layout(constant_id = 1) const float kSpecWindScale = 1.0;
layout(constant_id = 2) const float kSpecFootPush = 1.0;

const float kInteractionSoftness = 0.08;
const float kGrassMaskStart = 0.18;
const float kGrassMaskEnd = 0.34;
const float kTipFlexStart = 0.20;
const float kTipFlexEnd = 0.96;
const float kFootPressStart = 0.28;
const float kFootPressEnd = 0.98;
const float kFootHorizontalPush = 0.060;
const float kFootVerticalPush = 0.115;
const float kFootWindSuppression = 0.88;

float hash12(vec2 value) {
    return fract(sin(dot(value, vec2(127.1, 311.7))) * 43758.5453123);
}

vec2 gradient2(vec2 cell) {
    float angle = hash12(cell) * 6.28318530718;
    return vec2(cos(angle), sin(angle));
}

float gradientNoise(vec2 position) {
    vec2 cell = floor(position);
    vec2 local = fract(position);

    vec2 blend = local * local * (3.0 - 2.0 * local);

    float a = dot(gradient2(cell + vec2(0.0, 0.0)), local - vec2(0.0, 0.0));
    float b = dot(gradient2(cell + vec2(1.0, 0.0)), local - vec2(1.0, 0.0));
    float c = dot(gradient2(cell + vec2(0.0, 1.0)), local - vec2(0.0, 1.0));
    float d = dot(gradient2(cell + vec2(1.0, 1.0)), local - vec2(1.0, 1.0));

    return mix(mix(a, b, blend.x), mix(c, d, blend.x), blend.y);
}

float fbmNoise(vec2 position) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int octave = 0; octave < 3; ++octave) {
        value += gradientNoise(position * frequency) * amplitude;
        frequency *= 2.03;
        amplitude *= 0.5;
    }

    return value;
}

float computeFootInfluence(vec4 interaction, float screenX) {
    float radius = max(interaction.y, 0.0001);
    float distanceFromFoot = abs(screenX - interaction.x);
    float falloff = 1.0 - smoothstep(radius, radius + kInteractionSoftness, distanceFromFoot);
    return interaction.z * falloff;
}

float computeTipFlex(float localHeight) {
    float heightWeight = smoothstep(kTipFlexStart, kTipFlexEnd, localHeight);
    return inWindWeight * heightWeight;
}

vec2 applyWindDisplacement(vec2 position, float flex, float footBlend) {
    vec2 windDir = normalize(vec2(sign(windUbo.wind.x == 0.0 ? 1.0 : windUbo.wind.x), 0.35));
    vec2 samplePosition = vec2(
        position.x * (3.4 * kSpecWindScale),
        (1.0 - inTexCoord.y) * 2.1 + inWindPhase * 0.19);
    samplePosition += windDir * (windUbo.motion.x * 0.35);

    float lowBand = fbmNoise(samplePosition);
    float highBand = gradientNoise(samplePosition * 2.1 + vec2(0.0, windUbo.motion.x * 0.65));
    float phase = dot(position, windDir) * 6.0 + windUbo.motion.x * windUbo.motion.z + inWindPhase;
    float gerstnerLike = sin(phase) * 0.68 + cos(phase * 0.63 + lowBand * 1.7) * 0.32;
    float organicWave = mix(gerstnerLike, lowBand + highBand * 0.5, 0.42);

    float windAmplitude = windUbo.motion.y * kSpecWindStrength * inWindResponse * flex;
    windAmplitude *= (1.0 - footBlend * kFootWindSuppression);

    vec2 displaced = position;
    displaced.x += organicWave * windDir.x * windAmplitude;
    displaced.y += abs(organicWave) * windUbo.wind.y * windUbo.motion.w * windAmplitude;
    return displaced;
}

vec2 applyFootDisplacement(vec2 position, float localHeight) {
    float flex = computeTipFlex(localHeight);
    if (flex <= 0.0) {
        return vec2(0.0);
    }

    vec2 totalOffset = vec2(0.0);
    vec4 interactions[2] = vec4[2](windUbo.interactionA, windUbo.interactionB);

    for (int index = 0; index < 2; ++index) {
        vec4 interaction = interactions[index];
        float radius = max(interaction.y, 0.0001);
        float distanceFromFoot = abs(position.x - interaction.x);
        float radial = 1.0 - smoothstep(radius, radius + kInteractionSoftness, distanceFromFoot);
        if (radial <= 0.0) {
            continue;
        }

        float normalizedDistance = clamp(distanceFromFoot / radius, 0.0, 1.0);
        float sphereProfile = sqrt(max(0.0, 1.0 - normalizedDistance * normalizedDistance));
        float pressProfile = smoothstep(kFootPressStart, kFootPressEnd, localHeight);
        float influence = interaction.z * sphereProfile * pressProfile * flex * kSpecFootPush;

        float horizontalDirection = sign(position.x - interaction.x);
        if (horizontalDirection == 0.0) {
            horizontalDirection = sign(windUbo.wind.x == 0.0 ? 1.0 : windUbo.wind.x);
        }

        totalOffset.x += horizontalDirection * influence * kFootHorizontalPush;
        totalOffset.y += influence * kFootVerticalPush;
    }

    return totalOffset;
}

void main() {
    vec2 position = inPosition;
    float localHeight = 1.0 - inTexCoord.y;
    float tipFlex = computeTipFlex(localHeight);

    float footBlend = max(
        computeFootInfluence(windUbo.interactionA, inPosition.x),
        computeFootInfluence(windUbo.interactionB, inPosition.x));

    float grassMask = smoothstep(kGrassMaskStart, kGrassMaskEnd, localHeight);
    position = applyWindDisplacement(position, tipFlex * grassMask, footBlend);
    position += applyFootDisplacement(position, localHeight);

    gl_Position = vec4(position, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragAlpha = inAlpha;
    fragWindPhase = inWindPhase;
    fragMaterialType = inMaterialType;
    fragScreenX = position.x;
}
