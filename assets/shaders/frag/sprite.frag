#version 450
layout(early_fragment_tests) in;

const float kGrassDensity = 1.5;
const float kGrassDetailScale = 1.0;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in float fragAlpha;
layout(location = 2) in float fragWindPhase;
layout(location = 3) in float fragMaterialType;
layout(location = 4) in float fragScreenX;

layout(binding = 0) uniform sampler2D texSampler;
layout(location = 0) out vec4 outColor;

float hash(float n) { return fract(sin(n) * 43758.5453); }

float noise(vec2 uv) {
    vec2 i = floor(uv); vec2 f = fract(uv);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float n = i.x + i.y * 53.0;
    return mix(mix(hash(n), hash(n + 1.0), u.x), mix(hash(n + 53.0), hash(n + 54.0), u.x), u.y);
}

// --- Single grass blade ---
// layerSeed differentiates blade sets across depth layers
float blade(vec2 uv, float id, float density, float widthBias, float layerSeed) {
    float x = uv.x * density * kGrassDensity;
    float cell = floor(x);
    float lX = fract(x) - 0.5;
    float seed = id + cell * 13.0 + layerSeed * 47.0;

    // Random per-blade properties
    float h = mix(0.44, 1.16, hash(seed + 0.4));
    float p = clamp(uv.y / max(h, 0.01), 0.0, 1.0);
    float pC = p * p; // quadratic height curve

    // Width: wide base, aggressive taper toward tip
    float baseW = mix(0.016, 0.078, hash(seed + 2.0))
                * mix(0.78, 1.32, widthBias) * kGrassDetailScale;
    float taper = mix(1.28, 0.01, pow(p, 1.5));

    // Natural S-curve bend + waviness (curved even without wind)
    float bend = (hash(seed + 3.6) - 0.5) * 0.22 * pC
               + sin(seed * 0.73 + p * 4.0) * 0.018 * pC;

    // Blade width with slight bulge near lower-mid (natural leaf shape)
    float bulge = baseW * 0.06 * smoothstep(0.08, 0.35, p) * smoothstep(0.60, 0.35, p);
    float stemW = baseW * taper + bulge;

    // Anti-aliased edge
    float edge = max(0.01, fwidth(lX) * 2.0);
    float side = 1.0 - smoothstep(stemW, stemW + edge, abs(lX - bend));

    // Pointed tip
    float tW = baseW * mix(0.42, 1.08, hash(seed + 6.4));
    float tH = mix(0.022, 0.08, hash(seed + 8.7));
    float tDy = abs(uv.y - (h - tH * 0.5)) / max(tH, 0.001);
    float tip = (1.0 - smoothstep(0.85, 1.1,
                pow(abs(lX - bend) / max(tW, 0.001), 1.7) + tDy * tDy))
              * smoothstep(0.58, 0.90, p);

    return max(side * (1.0 - smoothstep(h - 0.035, h + 0.01, uv.y)), tip)
         * smoothstep(0.0, 0.02, uv.y);
}

void main() {
    // Sprite
    if (fragMaterialType <= 0.5) {
        vec4 tex = texture(texSampler, fragTexCoord);
        if (tex.a < 0.01) discard;
        outColor = vec4(tex.rgb, tex.a * fragAlpha);
        return;
    }

    // Soil
    if (fragMaterialType > 1.5) {
        vec2 uv = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
        float n = noise(uv * 6.0);
        outColor = vec4(mix(vec3(0.17, 0.1, 0.05), vec3(0.29, 0.18, 0.09), uv.y)
                      * (0.92 + n * 0.1), fragAlpha);
        return;
    }

    // === GRASS ===
    vec2 uv = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    float dN = noise(vec2(uv.x * 4.0 + fragWindPhase * 0.17, uv.y * 1.3));

    // Layer 1: Foreground — wide, tall blades
    float s1 = blade(uv, fragWindPhase + 1.0, 10.0 + dN * 2.0, 0.18, 0.0);

    // Layer 2: Midground — offset, slightly thinner
    float s2 = blade(vec2(fract(uv.x + 0.19), uv.y * 0.97),
                     fragWindPhase + 9.0, 13.0, 0.52, 1.0);

    // Layer 3: Background — dense, thin, shorter (adds depth)
    float s3 = blade(vec2(fract(uv.x + 0.43), uv.y * 0.88),
                     fragWindPhase + 17.0, 16.0, 0.78, 2.0);

    // Composite: foreground dominant, background dimmer
    float s = s1;
    s = max(s, s2 * 0.82);
    s = max(s, s3 * 0.62);

    if (s < 0.05) discard;

    // === VERTICAL COLOR GRADIENT ===
    vec3 baseCol = vec3(0.022, 0.052, 0.012);
    vec3 midCol  = vec3(0.072, 0.19,  0.032);
    vec3 upCol   = vec3(0.17,  0.37,  0.075);
    vec3 tipCol  = vec3(0.40,  0.55,  0.12);

    float h = uv.y;
    vec3 col;
    if (h < 0.25)      col = mix(baseCol, midCol, h / 0.25);
    else if (h < 0.55) col = mix(midCol,  upCol,  (h - 0.25) / 0.30);
    else                col = mix(upCol,   tipCol, (h - 0.55) / 0.45);

    // === DIFFUSE SHADING (top-down directional light) ===
    float diffuse = mix(0.48, 1.0, smoothstep(0.0, 0.8, h));
    diffuse *= 0.95 + 0.05 * sin(uv.x * 15.0);

    // === RIM LIGHT (depth at tips) ===
    float rim = smoothstep(0.65, 1.0, h) * 0.18;
    vec3 rimCol = vec3(0.48, 0.62, 0.22);

    col = col * diffuse + rimCol * rim;
    col *= (0.93 + dN * 0.12);
    col *= mix(0.62, 1.0, smoothstep(0.0, 0.06, uv.y));

    outColor = vec4(col, s * fragAlpha);
}
