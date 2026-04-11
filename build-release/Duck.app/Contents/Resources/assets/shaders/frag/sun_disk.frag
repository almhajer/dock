#version 450

layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SunDiskUniformBuffer {
    vec4 sunDisk;            // xy = screen position, z = disk radius, w = corona radius
    vec4 sunColorIntensity;  // rgb = color, a = hdr intensity
    vec4 appearance;         // x = limb darkening, y = corona falloff, z = edge softness, w = time
} ubo;

const int MAX_CLOUD_OCCLUDERS = 16;
const float INV_LN2 = 1.4426950408889634;

layout(set = 0, binding = 1) uniform CloudOcclusionBuffer {
    vec4 params;
    vec4 cloudRects[MAX_CLOUD_OCCLUDERS];
    vec4 cloudShape[MAX_CLOUD_OCCLUDERS];
} clouds;

float sat(float x)
{
    return clamp(x, 0.0, 1.0);
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

float fbmFast(vec2 uv)
{
    float value = 0.0;
    float amplitude = 0.5;

    // نقلنا التشويه إلى نسخة أخف: 3 طبقات فقط بدل 4.
    for (int i = 0; i < 3; ++i)
    {
        value += amplitude * noiseFast(uv);
        uv *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

float pow5(float x)
{
    float x2 = x * x;
    float x4 = x2 * x2;
    return x4 * x;
}

float pow7(float x)
{
    float x2 = x * x;
    float x4 = x2 * x2;
    return x4 * x2 * x;
}

float pow13(float x)
{
    float x2 = x * x;
    float x4 = x2 * x2;
    float x8 = x4 * x4;
    return x8 * x4 * x;
}

float pow19(float x)
{
    float x2 = x * x;
    float x4 = x2 * x2;
    float x8 = x4 * x4;
    float x16 = x8 * x8;
    return x16 * x2 * x;
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

float cloudOcclusionAtFast(vec2 uv)
{
    int count = clamp(int(clouds.params.x + 0.5), 0, MAX_CLOUD_OCCLUDERS);
    float alphaScale = clouds.params.y;
    float mask = 0.0;

    for (int i = 0; i < MAX_CLOUD_OCCLUDERS; ++i)
    {
        if (i >= count)
            break;

        vec4 rect = clouds.cloudRects[i];
        vec4 shape = clouds.cloudShape[i];

        float alpha = sat(shape.x * alphaScale);
        if (alpha <= 0.001)
            continue;

        float phase = shape.y;
        float softness = max(shape.z * 0.20, 0.08);

        vec2 center = rect.xy;
        vec2 halfSize = rect.zw;
        float softC = softness * 1.1;
        vec2 influenceBounds = halfSize * vec2(1.16, 0.80) * (1.0 + softC);

        if (any(greaterThan(abs(uv - center), influenceBounds)))
            continue;

        float sinP   = sin(phase);
        float cosP07 = cos(phase * 0.7);
        float cosP13 = cos(phase * 1.3);

        float body = ellipseOcclusionSq(
            uv,
            center,
            halfSize * vec2(0.94, 0.66),
            softness
        );

        float crownA = ellipseOcclusionSq(
            uv,
            center + vec2(-halfSize.x * (0.22 + sinP * 0.02), -halfSize.y * 0.24),
            halfSize * vec2(0.28, 0.24),
            softC
        );

        float crownB = ellipseOcclusionSq(
            uv,
            center + vec2(0.0, -halfSize.y * (0.32 + cosP07 * 0.02)),
            halfSize * vec2(0.32, 0.26),
            softC
        );

        float crownC = ellipseOcclusionSq(
            uv,
            center + vec2(halfSize.x * (0.22 + cosP13 * 0.02), -halfSize.y * 0.22),
            halfSize * vec2(0.28, 0.24),
            softC
        );

        float cloud = max(body, max(crownA, max(crownB, crownC)));
        mask = max(mask, cloud * alpha);

        if (mask > 0.985)
            break;
    }

    return mask;
}

void main()
{
    float time = ubo.appearance.w;

    vec2 delta = vUv - ubo.sunDisk.xy;
    float dist2 = dot(delta, delta);
    float dist  = sqrt(dist2);

    float diskR   = max(ubo.sunDisk.z, 1e-4);
    float coronaR = max(ubo.sunDisk.w, diskR + 1e-4);
    float soft    = max(ubo.appearance.z, 1e-4);

    float invDiskR      = 1.0 / diskR;
    float invCoronaR    = 1.0 / coronaR;
    float coronaSpan    = max(coronaR - diskR, 1e-4);
    float invCoronaSpan = 1.0 / coronaSpan;
    float invOuterSpan  = 1.0 / max(coronaR * 2.5, 1e-4);

    float corona4 = coronaR * 4.0;
    float disk03  = diskR * 0.3;

    // التشويه الحراري يبقى حول القرص فقط كي لا يدفع تكلفة على الشاشة كلها.
    if (dist < corona4 && dist > disk03)
    {
        vec2 dUv = delta * 10.0 + vec2(time * 0.25, time * 0.18);
        float heatMask = smoothstep(corona4, diskR * 0.5, dist);
        float distort = fbmFast(dUv) * 0.002 * heatMask;

        float invDist = inversesqrt(max(dist2, 1e-8));
        delta += delta * invDist * distort;

        dist2 = dot(delta, delta);
        dist  = sqrt(dist2);
    }

    float diskT   = dist * invDiskR;
    float coronaT = clamp((dist - diskR) * invCoronaSpan, 0.0, 1.0);
    float outerT  = clamp((dist - coronaR) * invOuterSpan, 0.0, 1.0);

    float disk = 1.0 - smoothstep(1.0 - soft, 1.0 + soft, diskT);
    float mu   = sqrt(max(0.0, 1.0 - diskT * diskT));
    float limb = mix(1.0 - ubo.appearance.x, 1.0, mu);

    vec3 coreC = vec3(1.0, 1.0, 0.97);
    vec3 midC  = vec3(1.0, 0.95, 0.80);
    vec3 edgeC = vec3(1.0, 0.86, 0.60);

    float cT = clamp(diskT, 0.0, 1.0);
    vec3 diskCol = mix(coreC, midC, smoothstep(0.0, 0.45, cT));
    diskCol = mix(diskCol, edgeC, smoothstep(0.45, 1.0, cT));
    diskCol *= limb;

    float outsideDisk = step(diskR, dist);
    float fall = ubo.appearance.y;
    float innerH = exp2(-coronaT * fall * 1.8 * INV_LN2) * outsideDisk;
    float midH   = exp2(-coronaT * fall * 0.8 * INV_LN2) * outsideDisk * 0.30;
    float outerH = exp2(-coronaT * fall * 0.35 * INV_LN2) * outsideDisk * 0.10;
    float atmos  = exp2(-outerT * 3.5 * INV_LN2) * 0.05;
    float halo   = innerH + midH + outerH + atmos;

    vec3 haloCol = mix(
        vec3(1.0, 0.90, 0.68),
        vec3(0.65, 0.78, 1.0),
        coronaT
    );

    float angle = atan(delta.y, delta.x);

    float c1 = max(0.0, cos(angle * 5.0  + 0.35));
    float c2 = max(0.0, cos(angle * 9.0  - 0.65));
    float c3 = max(0.0, cos(angle * 14.0 + 1.4));
    float c4 = max(0.0, cos(angle * 3.0  - 0.15));

    float rays =
          pow7(c1)
        + pow13(c2) * 0.55
        + pow19(c3) * 0.25
        + pow5(c4) * 0.35;

    float rayN = noiseFast(vec2(angle * 3.5, dist * 5.0));
    rays *= 0.55 + 0.45 * rayN;

    float rayFade = smoothstep(0.03, 0.20, dist * invCoronaR);
    rays *= rayFade * exp2(-dist / max(coronaR * 0.85, 1e-4) * INV_LN2);
    rays *= 1.0 + 0.06 * sin(time * 0.4 + angle * 2.0);

    float cloudOcclusion = cloudOcclusionAtFast(vUv);
    float diskTransmittance = 1.0 - cloudOcclusion;
    float haloTransmittance = 1.0 - cloudOcclusion * 0.88;
    float rayTransmittance  = 1.0 - cloudOcclusion * 0.82;
    disk *= diskTransmittance * diskTransmittance;
    halo *= haloTransmittance;
    rays *= rayTransmittance;

    float intensity = disk + halo + rays * 0.38;
    vec3 color = diskCol * disk
               + haloCol * halo
               + vec3(1.0, 0.90, 0.72) * rays * 0.38;

    float hdr = ubo.sunColorIntensity.a;
    color *= hdr;
    intensity = min(intensity * hdr, 60.0);

    outColor = vec4(color, intensity);
}
