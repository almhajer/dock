#version 450
layout(early_fragment_tests) in;

layout(location = 0) in vec2 atlasUv;
layout(location = 1) in vec2 localUv;
layout(location = 2) flat in vec4 tintAlpha;
layout(location = 3) flat in vec4 treeParams;
layout(location = 4) flat in float windBlend;

layout(binding = 0) uniform sampler2D atlasSampler;

layout(location = 0) out vec4 outColor;

const float ALPHA_CUTOFF = 0.02;
const vec3 TRUNK_BASE_COLOR = vec3(0.72, 0.70, 0.66);
const vec3 TRUNK_LIGHT_RANGE = vec3(0.32, 0.30, 0.26);

void main()
{
    vec4 atlasSample = texture(atlasSampler, atlasUv);
    float sampleAlpha = atlasSample.a;
    if (sampleAlpha < ALPHA_CUTOFF)
    {
        discard;
    }

    float localY = localUv.y;
    float invLocalY = 1.0 - localY;

    // التاج والجذع أصبحا في مسار مستقل، لذلك لا يوجد تفرع بين نوعين مختلفين هنا.
    float trunkMask = smoothstep(0.03, 0.18, atlasSample.r - atlasSample.g + 0.12);
    trunkMask *= smoothstep(0.22, 0.92, localY);

    float canopyTop = smoothstep(0.02, 0.88, invLocalY);
    float sideLight = smoothstep(0.92, 0.14, abs(localUv.x - 0.40) * 1.9);
    float innerShade = smoothstep(0.22, 0.88, localY);

    vec3 tinted = atlasSample.rgb * tintAlpha.rgb;

    float canopyLight = 0.78 + 0.28 * (canopyTop * 0.55 + sideLight * 0.30 + treeParams.x * 0.10);
    float canopyShade = 1.0 - innerShade * 0.10;
    vec3 canopyColor = tinted * (canopyLight * canopyShade);
    canopyColor += vec3(0.010, 0.016, 0.006) * canopyTop * (0.30 + windBlend * 0.16);

    float trunkLight = sideLight * 0.40 + invLocalY * 0.10;
    vec3 trunkColor = tinted;
    trunkColor *= TRUNK_BASE_COLOR + trunkLight * TRUNK_LIGHT_RANGE;
    trunkColor *= 1.0 - localY * 0.035;

    vec3 color = canopyColor + (trunkColor - canopyColor) * trunkMask;
    color *= 0.94 + (1.0 - trunkMask) * 0.06;

    outColor = vec4(color, sampleAlpha * tintAlpha.a);
}
