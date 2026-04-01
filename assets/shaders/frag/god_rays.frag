#version 450

layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sunMaskTex;

layout(set = 0, binding = 1) uniform GodRayUniformBuffer {
    vec4 sunPosDensityWeight;       // xy = sun position, z = density, w = weight
    vec4 decayExposureJitterSamples; // x = decay, y = exposure, z = jitter, w = num samples
} ubo;

const int kMaxRadialSamples = 96;

float spokeMask(vec2 delta)
{
    float angle = atan(delta.y, delta.x);
    float primary = pow(max(0.0, cos(angle * 6.0)), 8.0);
    float secondary = pow(max(0.0, cos(angle * 12.0 + 0.25)), 14.0) * 0.25;
    float diagonal = pow(max(0.0, cos(angle * 4.0 - 0.18)), 10.0) * 0.18;
    return primary + secondary + diagonal;
}

void main()
{
    int sampleCount = clamp(int(ubo.decayExposureJitterSamples.w), 8, kMaxRadialSamples);
    vec2 sunScreenPos = ubo.sunPosDensityWeight.xy;
    vec2 stepVector = (sunScreenPos - vUv) * (ubo.sunPosDensityWeight.z / float(sampleCount));
    vec2 sampleUv = vUv + stepVector * ubo.decayExposureJitterSamples.z;

    float illuminationDecay = 1.0;
    vec3 scattering = vec3(0.0);

    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
    {
        sampleUv += stepVector;
        float sampleMask = texture(sunMaskTex, sampleUv).r;
        scattering += vec3(sampleMask * illuminationDecay * ubo.sunPosDensityWeight.w);
        illuminationDecay *= ubo.decayExposureJitterSamples.x;
    }

    vec2 viewDelta = vUv - sunScreenPos;
    float distanceToSun = length(viewDelta);
    float spoke = spokeMask(viewDelta);
    float spokeBlend = mix(0.7, 1.0, clamp(spoke, 0.0, 1.0));
    float distanceFade = exp(-distanceToSun * 3.5);

    vec3 warmTint = vec3(1.0, 0.88, 0.65);
    float brightness = ubo.decayExposureJitterSamples.y * spokeBlend * (0.25 + distanceFade * 0.45);

    outColor = vec4(scattering * warmTint * brightness, 1.0);
}
