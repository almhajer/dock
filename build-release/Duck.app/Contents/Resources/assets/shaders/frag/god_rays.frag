#version 450

layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sunMaskTex;
layout(set = 0, binding = 1) uniform GodRayUniformBuffer {
    vec4 sunPosDensityWeight;        // xy = sun position, z = density, w = weight
    vec4 decayExposureJitterSamples; // x = decay, y = exposure, z = jitter, w = num samples
} ubo;

const int MAX_CLOUD_OCCLUDERS = 16;
const int MAX_SAMPLES = 48;

layout(set = 0, binding = 2) uniform CloudOcclusionBuffer {
    vec4 params;
    vec4 cloudRects[MAX_CLOUD_OCCLUDERS];
    vec4 cloudShape[MAX_CLOUD_OCCLUDERS];
} clouds;

float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

float pow4(float x)
{
    float x2 = x * x;
    return x2 * x2;
}

float pow6(float x)
{
    float x2 = x * x;
    return x2 * x2 * x2;
}

float pow12(float x)
{
    float x2 = x * x;
    float x4 = x2 * x2;
    float x8 = x4 * x4;
    return x8 * x4;
}

float pow18(float x)
{
    float x2 = x * x;
    float x4 = x2 * x2;
    float x8 = x4 * x4;
    float x16 = x8 * x8;
    return x16 * x2;
}

float hash12(vec2 p)
{
    vec3 q = fract(vec3(p.xyx) * 0.1031);
    q += dot(q, q.yzx + 33.33);
    return fract((q.x + q.y) * q.z);
}

float noiseFast(vec2 uv)
{
    vec2 i = floor(uv);
    vec2 f = fract(uv);
    vec2 u = f * f * (3.0 - 2.0 * f);

    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float ellipseOcclusionSq(vec2 uv, vec2 center, vec2 radii, float feather)
{
    vec2 invR = 1.0 / max(radii, vec2(1e-4));
    vec2 q = (uv - center) * invR;
    float dist2 = dot(q, q);

    float inner = max(1.0 - feather, 0.0);
    float outer = 1.0 + feather;
    inner *= inner;
    outer *= outer;

    return 1.0 - smoothstep(inner, outer, dist2);
}

vec3 cloudOcclusionAt3(vec2 uv0, vec2 uv1, vec2 uv2, int count)
{
    vec3 mask = vec3(0.0);
    float alphaScale = clouds.params.y;
    if (count <= 0 || alphaScale <= 0.001)
    {
        return mask;
    }

    for (int i = 0; i < MAX_CLOUD_OCCLUDERS; ++i)
    {
        if (i >= count)
            break;

        vec4 rect = clouds.cloudRects[i];
        vec4 shape = clouds.cloudShape[i];

        float alpha = saturate(shape.x * alphaScale);
        if (alpha <= 0.001)
            continue;

        float phase = shape.y;
        float softness = max(shape.z * 0.20, 0.08);

        vec2 halfSize = rect.zw;
        vec2 center   = rect.xy;

        vec2 bodyR   = halfSize * vec2(0.94, 0.66);
        vec2 crownAR = halfSize * vec2(0.28, 0.24);
        vec2 crownBR = halfSize * vec2(0.32, 0.26);
        vec2 crownCR = crownAR;

        float sinP   = sin(phase);
        float cosP07 = cos(phase * 0.7);
        float cosP13 = cos(phase * 1.3);

        vec2 crownAC = center + vec2(-halfSize.x * (0.22 + sinP   * 0.02), -halfSize.y * 0.24);
        vec2 crownBC = center + vec2( 0.0,                                -halfSize.y * (0.32 + cosP07 * 0.02));
        vec2 crownCC = center + vec2( halfSize.x * (0.22 + cosP13 * 0.02), -halfSize.y * 0.22);

        float softC = softness * 1.1;
        vec2 influenceBounds = halfSize * vec2(1.16, 0.80) * (1.0 + softC);

        bool active0 = all(lessThanEqual(abs(uv0 - center), influenceBounds));
        bool active1 = all(lessThanEqual(abs(uv1 - center), influenceBounds));
        bool active2 = all(lessThanEqual(abs(uv2 - center), influenceBounds));

        if (!active0 && !active1 && !active2)
            continue;

        float cloud0 = 0.0;
        float cloud1 = 0.0;
        float cloud2 = 0.0;

        if (active0)
        {
            cloud0 = max(
                ellipseOcclusionSq(uv0, center,  bodyR,   softness),
                max(
                    ellipseOcclusionSq(uv0, crownAC, crownAR, softC),
                    max(
                        ellipseOcclusionSq(uv0, crownBC, crownBR, softC),
                        ellipseOcclusionSq(uv0, crownCC, crownCR, softC)
                    )
                )
            );
        }

        if (active1)
        {
            cloud1 = max(
                ellipseOcclusionSq(uv1, center,  bodyR,   softness),
                max(
                    ellipseOcclusionSq(uv1, crownAC, crownAR, softC),
                    max(
                        ellipseOcclusionSq(uv1, crownBC, crownBR, softC),
                        ellipseOcclusionSq(uv1, crownCC, crownCR, softC)
                    )
                )
            );
        }

        if (active2)
        {
            cloud2 = max(
                ellipseOcclusionSq(uv2, center,  bodyR,   softness),
                max(
                    ellipseOcclusionSq(uv2, crownAC, crownAR, softC),
                    max(
                        ellipseOcclusionSq(uv2, crownBC, crownBR, softC),
                        ellipseOcclusionSq(uv2, crownCC, crownCR, softC)
                    )
                )
            );
        }

        mask = max(mask, vec3(cloud0, cloud1, cloud2) * alpha);

        if (mask.x > 0.95 && mask.y > 0.95 && mask.z > 0.95)
            break;
    }

    return clamp(mask, 0.0, 1.0);
}

void main()
{
    float exposure = ubo.decayExposureJitterSamples.y;
    float weight   = ubo.sunPosDensityWeight.w;

    if (exposure <= 1e-5 || weight <= 1e-5)
    {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    int samples = clamp(int(ubo.decayExposureJitterSamples.w + 0.5), 8, MAX_SAMPLES);

    vec2 sunPos = ubo.sunPosDensityWeight.xy;
    vec2 rayStep = (sunPos - vUv) * (ubo.sunPosDensityWeight.z / float(samples));
    vec2 uv = vUv + rayStep * ubo.decayExposureJitterSamples.z;

    float scatter = 0.0;
    float sampleWeight = weight;
    float decayMul = ubo.decayExposureJitterSamples.x;

    for (int i = 0; i < MAX_SAMPLES; ++i)
    {
        if (i >= samples)
            break;

        uv += rayStep;
        scatter += texture(sunMaskTex, uv).r * sampleWeight;
        sampleWeight *= decayMul;
    }

    float transmittance = 1.0;
    int cloudCount = clamp(int(clouds.params.x + 0.5), 0, MAX_CLOUD_OCCLUDERS);

    if (cloudCount > 0)
    {
        vec2 midUv = mix(vUv, sunPos, 0.50);
        vec3 occ = cloudOcclusionAt3(sunPos, midUv, vUv, cloudCount);

        float rayOcclusion = max(occ.x, max(occ.y * 0.72, occ.z * 0.46));
        transmittance = 1.0 - clamp(rayOcclusion, 0.0, 0.96);
        scatter *= transmittance;
    }

    vec2 d = vUv - sunPos;
    float angle = atan(d.y, d.x);
    float dist  = length(d);

    float s0 = max(0.0, cos(angle * 5.0  + 0.45));
    float s1 = max(0.0, cos(angle * 9.0  - 0.75));
    float s2 = max(0.0, cos(angle * 15.0 + 1.3));
    float s3 = max(0.0, cos(angle * 3.0  + 0.12));

    float spoke =
          pow6(s0)
        + pow12(s1) * 0.35
        + pow18(s2) * 0.18
        + pow4(s3)  * 0.22;

    float rayN = noiseFast(vec2(angle * 4.0, dist * 5.0));
    spoke *= 0.60 + 0.40 * rayN;

    float spokeBlend = mix(0.50, 1.0, saturate(spoke));

    float nearFade  = exp2(-dist * 4.61662413);
    float farFade   = exp2(-dist * 2.01977306) * 0.28;
    float totalFade = nearFade + farFade;

    float haze = exp2(-dist * 1.44269504) * scatter * 0.06;

    vec3 warmCol = vec3(1.0, 0.86, 0.58);
    vec3 coolCol = vec3(0.80, 0.85, 1.0);
    vec3 rayCol  = mix(warmCol, coolCol, smoothstep(0.0, 0.55, dist));

    float bright = exposure * spokeBlend * (0.18 + totalFade * 0.48);
    bright += haze * transmittance;
    bright *= 0.40 + 0.60 * transmittance;

    outColor = vec4(scatter * rayCol * bright, 1.0);
}
