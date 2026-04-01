#version 450

layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SunDiskUniformBuffer {
    vec4 sunDisk;            // xy = screen position, z = disk radius, w = corona radius
    vec4 sunColorIntensity;  // rgb = color, a = hdr intensity
    vec4 appearance;         // x = limb darkening, y = corona falloff, z = edge softness, w = time
} ubo;

float hash(float n) { return fract(sin(n) * 43758.5453); }

float noise(vec2 uv) {
    vec2 i = floor(uv); vec2 f = fract(uv);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float n = i.x + i.y * 57.0;
    return mix(mix(hash(n), hash(n + 1.0), u.x), mix(hash(n + 57.0), hash(n + 58.0), u.x), u.y);
}

float fbm(vec2 uv) {
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 4; ++i) { v += a * noise(uv); uv *= 2.0; a *= 0.5; }
    return v;
}

void main()
{
    float time = ubo.appearance.w;
    vec2 delta = vUv - ubo.sunDisk.xy;
    float dist = length(delta);
    float diskR = max(ubo.sunDisk.z, 1e-4);
    float coronaR = max(ubo.sunDisk.w, diskR + 1e-4);
    float soft = max(ubo.appearance.z, 1e-4);

    // === HEAT DISTORTION ===
    if (dist < coronaR * 4.0 && dist > diskR * 0.3) {
        vec2 dUv = delta * 10.0 + vec2(time * 0.25, time * 0.18);
        float distort = fbm(dUv) * 0.002 * smoothstep(coronaR * 4.0, diskR * 0.5, dist);
        delta += normalize(delta + 1e-6) * distort;
        dist = length(delta);
    }

    float diskT    = dist / diskR;
    float coronaT  = clamp((dist - diskR) / max(coronaR - diskR, 1e-4), 0.0, 1.0);
    float outerT   = clamp((dist - coronaR) / max(coronaR * 2.5, 1e-4), 0.0, 1.0);

    // === SOLAR DISC with color variation ===
    float disk = 1.0 - smoothstep(1.0 - soft, 1.0 + soft, diskT);
    float mu   = sqrt(max(0.0, 1.0 - diskT * diskT));
    float limb = mix(1.0 - ubo.appearance.x, 1.0, mu);

    // White-hot center → warm yellow → subtle orange edge
    vec3 coreC = vec3(1.0, 1.0, 0.97);
    vec3 midC  = vec3(1.0, 0.95, 0.80);
    vec3 edgeC = vec3(1.0, 0.86, 0.60);
    float cT = clamp(diskT, 0.0, 1.0);
    vec3 diskCol = mix(coreC, midC, smoothstep(0.0, 0.45, cT));
    diskCol = mix(diskCol, edgeC, smoothstep(0.45, 1.0, cT));
    diskCol *= limb;

    // === MULTI-LAYERED HALO ===
    float innerH = exp(-coronaT * ubo.appearance.y * 1.8) * step(diskR, dist);
    float midH   = exp(-coronaT * ubo.appearance.y * 0.8) * step(diskR, dist) * 0.30;
    float outerH = exp(-coronaT * ubo.appearance.y * 0.35) * step(diskR, dist) * 0.10;
    float atmos  = exp(-outerT * 3.5) * 0.05;
    float halo   = innerH + midH + outerH + atmos;

    // Warm near disk → atmospheric blue far out
    vec3 haloCol = mix(vec3(1.0, 0.90, 0.68),
                       vec3(0.65, 0.78, 1.0),
                       smoothstep(0.0, 1.0, coronaT));

    // === PROCEDURAL LIGHT RAYS ===
    float angle = atan(delta.y, delta.x);

    float r1 = pow(max(0.0, cos(angle * 5.0 + 0.35)), 7.0);
    float r2 = pow(max(0.0, cos(angle * 9.0 - 0.65)), 13.0) * 0.55;
    float r3 = pow(max(0.0, cos(angle * 14.0 + 1.4)), 19.0) * 0.25;
    float r4 = pow(max(0.0, cos(angle * 3.0 - 0.15)), 5.0) * 0.35;
    float rays = r1 + r2 + r3 + r4;

    // Noise modulation to break symmetry
    float rayN = noise(vec2(angle * 3.5, dist * 5.0));
    rays *= mix(0.55, 1.0, rayN);

    // Distance fade + subtle animated fluctuation
    float rayFade = smoothstep(0.03, 0.20, dist / max(coronaR, 1e-4));
    rays *= rayFade * exp(-dist / max(coronaR * 0.85, 1e-4));
    rays *= 1.0 + 0.06 * sin(time * 0.4 + angle * 2.0);

    // === COMBINE ===
    float intensity = disk + halo + rays * 0.38;
    vec3 color = diskCol * disk
               + haloCol * halo
               + vec3(1.0, 0.90, 0.72) * rays * 0.38;

    // HDR output
    float hdr = ubo.sunColorIntensity.a;
    color *= hdr;
    intensity = min(intensity * hdr, 60.0);

    outColor = vec4(color, intensity);
}
