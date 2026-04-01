#version 450

layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SunDiskUniformBuffer {
    vec4 sunDisk;            // xy = screen position, z = disk radius, w = corona radius
    vec4 sunColorIntensity;  // rgb = sun color, a = hdr intensity
    vec4 appearance;         // x = limb darkening, y = corona falloff, z = edge softness, w = mask only
} ubo;

float saturate(float value)
{
    return clamp(value, 0.0, 1.0);
}

float starburst(vec2 delta, float distanceToSun, float coronaRadius)
{
    float angle = atan(delta.y, delta.x);
    float primary = pow(max(0.0, cos(angle * 6.0)), 10.0);
    float secondary = pow(max(0.0, cos(angle * 12.0 + 0.35)), 16.0) * 0.3;
    float diagonal = pow(max(0.0, cos(angle * 4.0 - 0.2)), 12.0) * 0.18;
    float burst = primary + secondary + diagonal;

    float innerFade = smoothstep(0.04, 0.18, distanceToSun / max(coronaRadius, 1e-4));
    float radialFade = exp(-distanceToSun / max(coronaRadius * 0.85, 1e-4));
    return burst * innerFade * radialFade;
}

void main()
{
    vec2 delta = vUv - ubo.sunDisk.xy;
    float distanceToSun = length(delta);

    float diskRadius = max(ubo.sunDisk.z, 1e-4);
    float coronaRadius = max(ubo.sunDisk.w, diskRadius + 1e-4);
    float diskSoftness = max(ubo.appearance.z, 1e-4);

    float diskT = distanceToSun / diskRadius;
    float diskMask = 1.0 - smoothstep(1.0 - diskSoftness, 1.0 + diskSoftness, diskT);

    float mu = sqrt(max(0.0, 1.0 - diskT * diskT));
    float limb = mix(1.0 - ubo.appearance.x, 1.0, mu);

    float coronaT = saturate((distanceToSun - diskRadius) / max(coronaRadius - diskRadius, 1e-4));
    float corona = exp(-coronaT * ubo.appearance.y) * step(diskRadius, distanceToSun);
    float burst = starburst(delta, distanceToSun, coronaRadius);

    float intensity = diskMask * limb + corona * 0.12 + burst * 0.35;
    float maskOnly = step(0.5, ubo.appearance.w);

    vec3 hdrSun = ubo.sunColorIntensity.rgb * ubo.sunColorIntensity.a * intensity;
    vec3 maskValue = vec3(max(max(diskMask, corona * 0.65), burst));

    outColor = vec4(mix(hdrSun, maskValue, maskOnly), intensity);
}
