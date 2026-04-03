#version 450
layout(early_fragment_tests) in;

layout(location = 0) in vec2 atlasUv;
layout(location = 1) in vec2 localUv;
layout(location = 2) in vec4 tintAlpha;
layout(location = 3) in vec4 shape;
layout(location = 4) in vec2 fieldCoord;
layout(location = 5) in float windBlend;

layout(binding = 0) uniform sampler2D atlasSampler;

layout(location = 0) out vec4 outColor;

const float ALPHA_CUTOFF = 0.02;
const vec2 CLOUD_DETAIL_SCALE = vec2(4.2, 6.6);
const vec2 TREE_DETAIL_SCALE = vec2(6.1, 8.4);
const vec3 CLOUD_HIGHLIGHT_TINT = vec3(0.018, 0.016, 0.012);
const vec3 TRUNK_BASE_COLOR = vec3(0.72, 0.70, 0.66);
const vec3 TRUNK_LIGHT_RANGE = vec3(0.32, 0.30, 0.26);

float hash12(vec2 p)
{
    vec3 q = fract(vec3(p.xyx) * 0.1031);
    q += dot(q, q.yzx + 33.33);
    return fract((q.x + q.y) * q.z);
}

float saturate(float value)
{
    return clamp(value, 0.0, 1.0);
}

void main()
{
    vec4 atlasSample = texture(atlasSampler, atlasUv);
    float sampleAlpha = atlasSample.a;
    if (sampleAlpha < ALPHA_CUTOFF)
    {
        discard;
    }

    // القيم المشتركة تُحسب مرة واحدة لتخفيف تعليمات كل فرع.
    vec2 uv = localUv;
    float localY = uv.y;
    float invLocalY = 1.0 - localY;
    vec3 tinted = atlasSample.rgb * tintAlpha.rgb;
    float alpha = sampleAlpha * tintAlpha.a;
    vec3 color;

    if (shape.x < 0.5)
    {
        // الغيمة تستخدم إضاءة علوية وظلًا سفليًا، لكن بصياغة حسابية أخف من النسخة الأصلية.
        float topLight = smoothstep(0.94, 0.10, localY);
        float underside = smoothstep(0.42, 0.98, localY);
        float sideLight = smoothstep(0.96, 0.12, abs(uv.x - 0.50) * 1.70);
        float detail = hash12(fieldCoord * 6.1 + uv * CLOUD_DETAIL_SCALE);
        float interior = saturate(sampleAlpha * 0.70 + sideLight * 0.20 + topLight * 0.10);
        float highlight = topLight * 0.46 + sideLight * 0.18 + windBlend * 0.08;
        float shadow = underside * (0.54 - interior * 0.20);
        float brightness = 0.82 + 0.28 * (interior + highlight * 0.35 + detail * 0.04);

        color = tinted * brightness;
        color *= vec3(1.0 - shadow * 0.16, 1.0 - shadow * 0.11, 1.0 - shadow * 0.05);
        color += CLOUD_HIGHLIGHT_TINT * highlight;
        alpha *= (0.86 + 0.18 * shape.y) * (0.90 + sampleAlpha * 0.08);
    }
    else
    {
        // الشجرة تبقي الأوراق لينة، لكن الجذع يُحسب بمسار أبسط وأكثر مباشرة.
        float trunkMask = smoothstep(0.03, 0.18, atlasSample.r - atlasSample.g + 0.12);
        trunkMask *= smoothstep(0.22, 0.92, localY);

        float canopyTop = smoothstep(0.02, 0.88, invLocalY);
        float sideLight = smoothstep(0.92, 0.14, abs(uv.x - 0.40) * 1.9);
        float innerShade = smoothstep(0.22, 0.88, localY);
        float detail = hash12(fieldCoord * 8.7 + uv * TREE_DETAIL_SCALE);

        float canopyLight = 0.74 + 0.40 * (canopyTop * 0.46 + sideLight * 0.30 + detail * 0.08);
        float canopyShade = 1.0 - innerShade * 0.0748;
        vec3 canopyColor = tinted * (canopyLight * canopyShade);
        canopyColor += vec3(0.010, 0.016, 0.006) * canopyTop * (0.40 + windBlend * 0.20);

        float trunkLight = sideLight * 0.40 + invLocalY * 0.10;
        vec3 trunkColor = tinted;
        trunkColor *= TRUNK_BASE_COLOR + trunkLight * TRUNK_LIGHT_RANGE;
        trunkColor *= 1.0 - localY * 0.0352;

        color = canopyColor + (trunkColor - canopyColor) * trunkMask;
        color *= 1.0 - trunkMask * 0.06;
    }

    outColor = vec4(color, alpha);
}
