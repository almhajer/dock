#version 450

layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sunMaskTex;
layout(set = 0, binding = 1) uniform GodRayUniformBuffer {
    vec4 sunPosDensityWeight;        // xy = sun position, z = density, w = weight
    vec4 decayExposureJitterSamples; // x = decay, y = exposure, z = jitter, w = num samples
} ubo;

float hash(float n) { return fract(sin(n) * 43758.5453); }

float noise(vec2 uv) {
    vec2 i = floor(uv); vec2 f = fract(uv);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float n = i.x + i.y * 57.0;
    return mix(mix(hash(n), hash(n + 1.0), u.x), mix(hash(n + 57.0), hash(n + 58.0), u.x), u.y);
}

void main()
{
    int samples = clamp(int(ubo.decayExposureJitterSamples.w), 8, 48);
    vec2 sunPos = ubo.sunPosDensityWeight.xy;
    vec2 step = (sunPos - vUv) * (ubo.sunPosDensityWeight.z / float(samples));
    vec2 uv = vUv + step * ubo.decayExposureJitterSamples.z;

    float decay = 1.0;
    float scatter = 0.0;
    float weight = ubo.sunPosDensityWeight.w;

    for (int i = 0; i < samples; ++i)
    {
        uv += step;
        scatter += texture(sunMaskTex, uv).r * decay * weight;
        decay *= ubo.decayExposureJitterSamples.x;
    }

    // === ORGANIC ANGULAR RAY MODULATION ===
    vec2 d = vUv - sunPos;
    float angle = atan(d.y, d.x);
    float dist = length(d);

    float spoke = pow(max(0.0, cos(angle * 5.0 + 0.45)), 6.0)
                + pow(max(0.0, cos(angle * 9.0 - 0.75)), 12.0) * 0.35
                + pow(max(0.0, cos(angle * 15.0 + 1.3)), 18.0) * 0.18
                + pow(max(0.0, cos(angle * 3.0 + 0.12)), 4.0) * 0.22;

    // Noise to break ray uniformity
    float rayN = noise(vec2(angle * 4.0, dist * 5.0));
    spoke *= mix(0.60, 1.0, rayN);

    float spokeBlend = mix(0.50, 1.0, clamp(spoke, 0.0, 1.0));

    // === DISTANCE FALLOFF ===
    float nearFade = exp(-dist * 3.2);
    float farFade  = exp(-dist * 1.4) * 0.28;
    float totalFade = nearFade + farFade;

    // === ATMOSPHERIC HAZE ===
    float haze = exp(-dist * 1.0) * scatter * 0.06;

    // === COLOR: warm near sun → atmospheric cool with distance ===
    vec3 warmCol = vec3(1.0, 0.86, 0.58);
    vec3 coolCol = vec3(0.80, 0.85, 1.0);
    vec3 rayCol  = mix(warmCol, coolCol, smoothstep(0.0, 0.55, dist));

    float bright = ubo.decayExposureJitterSamples.y * spokeBlend
                 * (0.18 + totalFade * 0.48);
    bright += haze;

    outColor = vec4(scatter * rayCol * bright, 1.0);
}
