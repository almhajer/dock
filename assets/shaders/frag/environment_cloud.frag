#version 450
layout(early_fragment_tests) in;

layout(location = 0) in vec2 atlasUv;
layout(location = 1) in vec2 localUv;
layout(location = 2) flat in vec4 tintAlpha;
layout(location = 3) flat in vec2 fieldCoord;
layout(location = 4) flat in vec2 cloudShape;
layout(location = 5) flat in float windBlend;

layout(binding = 0) uniform sampler2D atlasSampler;

layout(location = 0) out vec4 outColor;

const float ALPHA_CUTOFF = 0.02;
const vec3 CLOUD_HIGHLIGHT_TINT = vec3(0.020, 0.018, 0.014);

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

    // تظليل سحابي خفيف يعتمد على التدرج والشفافية بدل ضوضاء لكل بكسل.
    float localY = localUv.y;
    float topLight = smoothstep(0.94, 0.10, localY);
    float underside = smoothstep(0.40, 0.98, localY);
    float sideMask = 1.0 - abs(localUv.x * 2.0 - 1.0);
    sideMask *= sideMask;

    float density = sampleAlpha;
    float volume = saturate(density * 0.72 + sideMask * (0.16 + cloudShape.y * 0.04) + topLight * 0.10);
    float shadow = underside * (0.30 + (1.0 - volume) * 0.18);
    float highlight = topLight * 0.34 + sideMask * 0.16 + windBlend * 0.06;

    vec3 color = atlasSample.rgb * tintAlpha.rgb;
    color *= 0.84 + 0.24 * volume + 0.10 * highlight;
    color *= vec3(1.0 - shadow * 0.17, 1.0 - shadow * 0.12, 1.0 - shadow * 0.05);
    color += CLOUD_HIGHLIGHT_TINT * highlight;

    float alpha = sampleAlpha * tintAlpha.a;
    alpha *= (0.90 + density * 0.06) * (0.88 + cloudShape.x * 0.12);

    outColor = vec4(color, alpha);
}
