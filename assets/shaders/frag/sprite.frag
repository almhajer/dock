#version 450

layout(early_fragment_tests) in;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in float fragAlpha;
layout(location = 2) in float fragWindPhase;
layout(location = 3) in float fragMaterialType;
layout(location = 4) in float fragScreenX;

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform WindUniformBuffer {
    vec4 motion;
    vec4 wind;
    vec4 interactionA;
    vec4 interactionB;
} windUbo;

layout(location = 0) out vec4 outColor;

// Specialization constants يمكن تمريرها من Vulkan لاحقًا من دون إعادة ترجمة الـ SPIR-V.
layout(constant_id = 10) const float kSpecBladeDensity = 1.0;
layout(constant_id = 11) const float kSpecAoStrength = 1.0;
layout(constant_id = 12) const float kSpecTipScatter = 1.0;

const float kMaterialGrassCutoff = 0.5;
const float kMaterialSoilCutoff = 1.5;
const float kAlphaDiscardThreshold = 0.02;
const float kFootSoftness = 0.08;
const float kGrassBase = 0.055;
const float kGrassGateStart = 0.055;
const float kGrassGateEnd = 0.069;
const float kRootBedStart = 0.067;
const float kRootBedEnd = 0.137;
const float kRootBedStrength = 0.62;
const int kGrassLayerCount = 5;

float hash1(float value) {
    return fract(sin(value) * 43758.5453123);
}

float safeDivisor(float value) {
    return max(value, 0.001);
}

vec2 proceduralUv() {
    return vec2(
        clamp(fragTexCoord.x, 0.0, 1.0),
        1.0 - clamp(fragTexCoord.y, 0.0, 1.0));
}

float layeredNoise(vec2 position) {
    vec2 cell = floor(position);
    vec2 local = fract(position);

    float a = hash1(cell.x + cell.y * 57.0);
    float b = hash1(cell.x + 1.0 + cell.y * 57.0);
    float c = hash1(cell.x + (cell.y + 1.0) * 57.0);
    float d = hash1(cell.x + 1.0 + (cell.y + 1.0) * 57.0);

    vec2 blend = local * local * (3.0 - 2.0 * local);
    return mix(mix(a, b, blend.x), mix(c, d, blend.x), blend.y);
}

float computeFootInfluence(vec4 interaction, float screenX) {
    float radius = max(interaction.y, 0.0001);
    float distanceFromFoot = abs(screenX - interaction.x);
    float falloff = 1.0 - smoothstep(radius, radius + kFootSoftness, distanceFromFoot);
    return interaction.z * falloff;
}

float combinedFootInfluence(float screenX) {
    float leftInfluence = computeFootInfluence(windUbo.interactionA, screenX);
    float rightInfluence = computeFootInfluence(windUbo.interactionB, screenX);
    return max(leftInfluence, rightInfluence);
}

float computeBladeHeight(float bladeSeed, float x) {
    float canopyLimit = mix(0.72, 1.08, hash1(bladeSeed + 19.1));
    float bladeHeight = mix(0.44, 1.16, hash1(bladeSeed + 0.7));
    bladeHeight *= mix(
        0.88,
        1.12,
        hash1(bladeSeed * 0.37 + floor(fract(x) * 5.0) + 2.4));
    return min(bladeHeight, canopyLimit);
}

float computeBladeSilhouette(vec2 uv, float seed, float density, float xOffset, float layerScale) {
    float x = uv.x * density + xOffset;
    float cell = floor(x);
    float localX = fract(x) - 0.5;
    float bladeSeed = seed + cell * 17.0;

    float bladeHeight = computeBladeHeight(bladeSeed, x) * layerScale;
    float bladeWidth = mix(0.020, 0.078, hash1(bladeSeed + 3.1));
    float progress = clamp(uv.y / safeDivisor(bladeHeight), 0.0, 1.0);

    float bend = (hash1(bladeSeed + 6.9) - 0.5) * 0.20 * progress;
    bend += sin(progress * 3.4 + bladeSeed) * 0.015 * progress;

    float fullness = hash1(bladeSeed + 8.9);
    float headWidth = bladeWidth * mix(0.35, 1.35, pow(fullness, 1.25));
    float shoulderMask = smoothstep(0.52, 0.84, progress) * (1.0 - smoothstep(0.88, 1.02, progress));
    float tipMask = smoothstep(0.70, 0.92, progress) * (1.0 - smoothstep(0.96, 1.04, progress));

    float widthAtHeight = bladeWidth * mix(1.16, 0.045, progress * progress);
    widthAtHeight += bladeWidth * mix(0.02, 0.30, fullness) * shoulderMask;
    widthAtHeight += headWidth * tipMask;

    float edgeSoftness = max(0.010, fwidth(abs(localX - bend)) * 1.6);
    float lateralMask = 1.0 - smoothstep(
        widthAtHeight,
        widthAtHeight + edgeSoftness,
        abs(localX - bend));

    float heightSoftness = max(0.018, fwidth(uv.y) * 2.0);
    float heightMask = 1.0 - smoothstep(bladeHeight - 0.05, bladeHeight + heightSoftness, uv.y);
    float rootMask = smoothstep(0.0, 0.035, uv.y + 0.002);

    float headHeight = mix(0.018, 0.072, hash1(bladeSeed + 12.7));
    float headCenterY = bladeHeight - headHeight * mix(0.52, 0.64, fullness);
    float headDx = abs(localX - bend) / safeDivisor(headWidth * mix(0.90, 1.25, fullness));
    float headDy = abs(uv.y - headCenterY) / safeDivisor(headHeight);
    float roundedHead = 1.0 - smoothstep(0.90, 1.08, pow(headDx, 1.7) + headDy * headDy);
    roundedHead *= smoothstep(0.66, 0.98, progress) * (0.30 + fullness * 0.70);

    return max(lateralMask, roundedHead) * heightMask * rootMask;
}

float sampleGrassSilhouette(vec2 uv, float seed, float densityMask) {
    float silhouette = 0.0;

    for (int layer = 0; layer < kGrassLayerCount; ++layer) {
        float layerT = float(layer) / float(kGrassLayerCount - 1);
        float layerSeed = seed + 13.0 * float(layer);
        float layerDensity = mix(9.8, 18.8, layerT) * densityMask * kSpecBladeDensity;
        float layerHeight = mix(1.00, 0.82, layerT);
        float layerAlpha = mix(1.00, 0.42, layerT);
        float layerOffset = fract(0.17 * float(layer) + sin(seed * 0.11 + float(layer) * 1.7) * 0.06);
        float xOffset = 0.11 * float(layer);

        vec2 layerUv = vec2(fract(uv.x + layerOffset), uv.y * layerHeight);
        float layerSilhouette = computeBladeSilhouette(layerUv, layerSeed, layerDensity, xOffset, 1.0);
        silhouette = max(silhouette, layerSilhouette * layerAlpha);
    }

    return silhouette;
}

float computeCanopyMask(vec2 uv, float seed) {
    float canopyNoiseA = layeredNoise(vec2(uv.x * 8.5 + seed * 0.021, 0.43));
    float canopyNoiseB = layeredNoise(vec2(uv.x * 20.0 + seed * 0.047, 0.79));
    float canopyHeight = 0.76 + canopyNoiseA * 0.18 + canopyNoiseB * 0.09;
    float edgeSoftness = max(0.05, fwidth(uv.y) * 3.0);
    return 1.0 - smoothstep(canopyHeight, canopyHeight + edgeSoftness, uv.y);
}

vec3 computeGrassColor(vec2 uv, float broadNoise, float fineNoise, float silhouette, float footBlend) {
    vec3 rootColor = vec3(0.04, 0.08, 0.025);
    vec3 baseColor = vec3(0.11, 0.19, 0.06);
    vec3 midColor = vec3(0.20, 0.34, 0.10);
    vec3 tipColor = vec3(0.42, 0.56, 0.20);
    vec3 dryTint = vec3(0.46, 0.40, 0.18);

    vec3 color = mix(rootColor, baseColor, smoothstep(0.10, 0.24, uv.y));
    color = mix(color, midColor, smoothstep(0.22, 0.60, uv.y));
    color = mix(color, tipColor, smoothstep(0.58, 1.0, uv.y));
    color = mix(color, dryTint, smoothstep(0.48, 1.0, uv.y) * (0.10 + broadNoise * 0.08));

    // Fake AO: الجذور أغمق لأن الضوء المحيط يصلها أقل.
    float ao = mix(0.68, 1.0, smoothstep(0.04, 0.42, uv.y));
    color *= mix(1.0, ao, kSpecAoStrength);

    // Fake subsurface scattering: الأطراف أفتح قليلًا كأن الضوء يمر خلالها.
    float scatter = smoothstep(0.58, 1.0, uv.y) * silhouette * (0.10 + broadNoise * 0.08);
    color += vec3(0.08, 0.10, 0.04) * scatter * kSpecTipScatter;

    float rootShadow = 1.0 - smoothstep(kGrassBase + 0.020, kGrassBase + 0.145, uv.y);
    color *= 1.0 - rootShadow * 0.34;
    color *= 0.96 + fineNoise * 0.08;

    float footMask = footBlend *
        smoothstep(kGrassBase + 0.040, kGrassBase + 0.095, uv.y) *
        (1.0 - smoothstep(kGrassBase + 0.150, kGrassBase + 0.290, uv.y));
    color *= 1.0 - footMask * 0.14;

    float phaseVariation = 0.88 + 0.12 * sin(fragWindPhase * 1.7 + uv.x * 8.0 + broadNoise * 2.0);
    float highlight = silhouette * smoothstep(0.55, 1.0, uv.y) * 0.06;
    color *= phaseVariation;
    color += vec3(0.05, 0.08, 0.03) * highlight;
    return color;
}

vec4 shadeProceduralGrass() {
    vec2 uv = proceduralUv();
    float seed = floor(fragWindPhase * 23.0);
    float broadNoise = layeredNoise(vec2(uv.x * 3.2 + seed * 0.011, uv.y * 1.7));
    float fineNoise = layeredNoise(vec2(uv.x * 14.0 + seed * 0.027, uv.y * 9.0));
    float densityMask = 0.86 + fineNoise * 0.06;
    float footBlend = combinedFootInfluence(fragScreenX);

    float rootBed = 1.0 - smoothstep(kRootBedStart, kRootBedEnd, uv.y);
    float grassGate = smoothstep(kGrassGateStart, kGrassGateEnd, uv.y);
    float bladeSilhouette = sampleGrassSilhouette(uv, seed, densityMask);
    bladeSilhouette *= computeCanopyMask(uv, seed);
    bladeSilhouette *= grassGate;

    float silhouette = max(rootBed * kRootBedStrength, bladeSilhouette);
    float alpha = silhouette * fragAlpha;
    if (alpha < kAlphaDiscardThreshold) {
        discard;
    }

    vec3 color = computeGrassColor(uv, broadNoise, fineNoise, bladeSilhouette, footBlend);
    return vec4(color, alpha);
}

vec4 shadeProceduralSoil() {
    vec2 uv = proceduralUv();
    float broadNoise = layeredNoise(vec2(uv.x * 2.8, uv.y * 1.7));
    float fineNoise = layeredNoise(vec2(uv.x * 14.0 + 3.1, uv.y * 18.0));

    vec3 soilDark = vec3(0.15, 0.09, 0.045);
    vec3 soilMid = vec3(0.28, 0.18, 0.095);
    vec3 soilWarm = vec3(0.36, 0.24, 0.12);

    float heightBlend = smoothstep(0.0, 1.0, uv.y);
    vec3 color = mix(soilDark, soilMid, heightBlend);
    color = mix(color, soilWarm, heightBlend * (0.18 + fineNoise * 0.10));

    float horizontalVariation = 0.92 + 0.08 * broadNoise;
    float grain = 0.94 + 0.10 * fineNoise;
    color *= horizontalVariation * grain;

    return vec4(color, fragAlpha);
}

vec4 shadeTexturedSprite() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    if (texColor.a < 0.01) {
        discard;
    }

    texColor.a *= fragAlpha;
    return texColor;
}

void main() {
    if (fragMaterialType > kMaterialGrassCutoff) {
        outColor = (fragMaterialType > kMaterialSoilCutoff)
            ? shadeProceduralSoil()
            : shadeProceduralGrass();
        return;
    }

    outColor = shadeTexturedSprite();
}
