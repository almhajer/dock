#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in float inAlpha;
layout(location = 3) in float inWindWeight;
layout(location = 4) in float inWindPhase;
layout(location = 5) in float inWindResponse;
layout(location = 6) in float inMaterialType;

layout(binding = 1) uniform WindUniformBuffer {
    vec4 motion;
    vec4 wind;
    vec4 interactionA;
    vec4 interactionB;
} ubo;

layout(binding = 2) uniform sampler2D windMap;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) flat out float fragAlpha;
layout(location = 2) flat out float fragWindPhase;
layout(location = 3) flat out float fragMaterialType;
layout(location = 4) out float fragFieldX;

const float TAU = 6.28318530718;

float hash11(float p)
{
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

void main()
{
    vec2 basePos = inPosition;
    vec2 pos = basePos;

    /*
     * العناصر الخامية العادية مثل الصياد والنصوص وHUD
     * يجب أن تمر بدون أي تشويه رياح أو تفاعل قدمين.
     * التشويه محصور في العشب والتربة فقط.
     */
    if (inMaterialType < 0.5)
    {
        gl_Position = vec4(pos, 0.0, 1.0);
        fragTexCoord = inTexCoord;
        fragAlpha = inAlpha;
        fragWindPhase = inWindPhase;
        fragMaterialType = inMaterialType;
        fragFieldX = basePos.x;
        return;
    }

    float time     = ubo.motion.x;
    float strength = ubo.motion.y;
    float speed    = ubo.motion.z;
    float sag      = ubo.motion.w;

    float localHeight = 1.0 - inTexCoord.y;

    float bendMask = smoothstep(0.10, 0.96, localHeight);
    bendMask *= bendMask;

    float tipMask  = smoothstep(0.50, 1.0, localHeight);
    float footMask = smoothstep(0.14, 0.96, localHeight);

    float phaseSeed = inWindPhase;

    float r0 = hash11(phaseSeed + 2.37);
    float r1 = hash11(phaseSeed + 5.91);
    float r2 = hash11(phaseSeed + 10.4);
    float r3 = hash11(phaseSeed + 14.2);

    float phase       = r0 * TAU;
    float baseLean    = (r1 - 0.5) * 0.018;
    float stiffness   = mix(0.86, 1.14, r2);
    float flexibility = mix(0.82, 1.20, r3);

    float response = inWindResponse * flexibility / stiffness;

    float t = time * speed;

    float macroWind =
        sin(t * 0.24 + basePos.x * 1.28 + phase) +
        0.26 * sin(t * 0.13 - basePos.x * 0.54 + phase * 1.4);
    macroWind *= 0.74;

    vec2 windUv = vec2(
        basePos.x * 0.24 + t * 0.012,
        phaseSeed * 0.041 + localHeight * 0.10
    );

    vec3 windSample = textureLod(windMap, windUv, 0.0).rgb * 2.0 - 1.0;

    float microWind =
        windSample.x * 0.34 +
        sin(t * 0.95 + phase * 2.2 + localHeight * 5.0) * 0.08;

    float horizontalOffset =
        ((macroWind + microWind) * strength * response + baseLean) * bendMask;

    vec2 interX     = vec2(ubo.interactionA.x, ubo.interactionB.x);
    vec2 interRange = vec2(ubo.interactionA.y, ubo.interactionB.y);
    vec2 interPower = vec2(ubo.interactionA.z, ubo.interactionB.z);

    vec2 interDist = abs(vec2(basePos.x) - interX);
    vec2 interFade = 1.0 - smoothstep(interRange, interRange + vec2(0.10), interDist);
    vec2 interMask = interFade * interPower;

    float footBlend = max(interMask.x, interMask.y) * footMask;

    if (inMaterialType > 1.5)
    {
        /*
         * التربة تستجيب فقط لضغط القدمين، من دون انحناء رياح.
         */
        float soilMask = smoothstep(0.12, 0.88, localHeight);
        pos.y += footBlend * soilMask * 0.035;
    }
    else
    {
        float dampen    = 1.0 - footBlend * 0.78;

        float absHorizontalOffset = abs(horizontalOffset);
        float verticalLift =
            (absHorizontalOffset * 0.62 + abs(windSample.y) * strength * 0.14) *
            sag * response * mix(0.22, 1.0, tipMask);

        pos.x += horizontalOffset * dampen;
        pos.y += verticalLift * dampen + footBlend * bendMask * 0.11;
    }

    gl_Position = vec4(pos, 0.0, 1.0);

    // هذه القيم ثابتة لكل رقعة، لذلك تمريرها flat يقلل كلفة الاستيفاء.
    fragTexCoord     = inTexCoord;
    fragAlpha        = inAlpha;
    fragWindPhase    = inWindPhase;
    fragMaterialType = inMaterialType;
    fragFieldX       = basePos.x;
}
