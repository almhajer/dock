#version 450
layout(early_fragment_tests) in;

// --- Specialization Constants (Vulkan IDs) ---
layout(constant_id = 0) const float kSpecGrassDensity = 1.0; // معامل كثافة العشب
layout(constant_id = 1) const int   kSpecMaxLayers = 3;      // عدد طبقات العشب (1-3)
layout(constant_id = 2) const float kSpecDetailScale = 1.0;  // دقة التفاصيل العرضية

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in float fragAlpha;
layout(location = 2) in float fragWindPhase;
layout(location = 3) in float fragMaterialType;
layout(location = 4) in float fragScreenX;

layout(binding = 0) uniform sampler2D texSampler;
layout(location = 0) out vec4 outColor;

const float kGrassCutoff = 0.5;
const float kSoilCutoff = 1.5;

float hash(float n) { return fract(sin(n) * 43758.5453); }

float fastNoise(vec2 uv) {
    vec2 i = floor(uv); vec2 f = fract(uv);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float n = i.x + i.y * 53.0;
    return mix(mix(hash(n), hash(n+1.0), u.x), mix(hash(n+53.0), hash(n+54.0), u.x), u.y);
}

float getBlade(vec2 uv, float id, float density, float widthBias) {
    float x = uv.x * (density * kSpecGrassDensity);
    float cell = floor(x);
    float lX = fract(x) - 0.5;
    float seed = id + cell * 13.0;

    float h = mix(0.52, 1.08, hash(seed + 0.4));
    float p = clamp(uv.y / max(h, 0.01), 0.0, 1.0);

    // العرض المتأثر بالـ Specialization Constant
    float baseWidth = mix(0.018, 0.082, hash(seed + 2.0)) * mix(0.82, 1.28, widthBias) * kSpecDetailScale;
    float bend = (hash(seed + 3.6) - 0.5) * 0.22 * p + sin(seed * 0.73 + p * 3.4) * 0.012 * p;

    float taper = mix(1.16, 0.02, pow(p, 1.35));
    float stemW = baseWidth * taper + (baseWidth * 0.1 * smoothstep(0.14, 0.56, p));

    float edge = max(0.01, fwidth(lX) * 2.1);
    float sideM = 1.0 - smoothstep(stemW, stemW + edge, abs(lX - bend));

    // Tip Logic
    float tW = baseWidth * mix(0.52, 1.12, hash(seed + 6.4));
    float tH = mix(0.028, 0.085, hash(seed + 8.7));
    float tDy = abs(uv.y - (h - tH * 0.5)) / max(tH, 0.001);
    float tipM = (1.0 - smoothstep(0.88, 1.1, pow(abs(lX-bend)/max(tW,0.001), 1.7) + tDy*tDy)) * smoothstep(0.62, 0.94, p);

    return max(sideM * (1.0 - smoothstep(h - 0.035, h + 0.01, uv.y)), tipM) * smoothstep(0.0, 0.02, uv.y);
}

void main() {
    // 1. التعامل مع السبرايت (Sprite)
    if (fragMaterialType <= kGrassCutoff) {
        vec4 tex = texture(texSampler, fragTexCoord);
        if (tex.a < 0.01) discard;
        outColor = vec4(tex.rgb, tex.a * fragAlpha);
        return;
    }

    // 2. التعامل مع التربة (Soil)
    if (fragMaterialType > kSoilCutoff) {
        vec2 uv = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
        float n = fastNoise(uv * 6.0);
        outColor = vec4(mix(vec3(0.17, 0.1, 0.05), vec3(0.29, 0.18, 0.09), uv.y) * (0.92 + n * 0.1), fragAlpha);
        return;
    }

    // 3. التعامل مع العشب الإجرائي (Grass)
    vec2 uv = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    float dNoise = fastNoise(vec2(uv.x * 4.0 + fragWindPhase * 0.17, uv.y * 1.3));

    // الطبقة الأولى (دائمة)
    float s = getBlade(uv, fragWindPhase + 1.0, 10.0 + dNoise * 2.0, 0.18);

    // طبقات إضافية مفعلة حسب kSpecMaxLayers
    if (kSpecMaxLayers >= 2)
        s = max(s, getBlade(vec2(fract(uv.x + 0.19), uv.y * 0.97), fragWindPhase + 9.0, 13.0, 0.52) * 0.8);
    if (kSpecMaxLayers >= 3)
        s = max(s, getBlade(vec2(fract(uv.x + 0.41), uv.y * 1.02), fragWindPhase + 17.0, 8.4, 0.86) * 0.68);

    if (s < 0.05) discard;

    vec3 col = mix(vec3(0.03, 0.07, 0.02), vec3(0.15, 0.3, 0.08), smoothstep(0.02, 0.45, uv.y));
    col = mix(col, vec3(0.36, 0.58, 0.14), smoothstep(0.42, 1.0, uv.y));
    outColor = vec4(col * mix(0.66, 1.0, uv.y) * (0.94 + dNoise * 0.1), s * fragAlpha);
}
