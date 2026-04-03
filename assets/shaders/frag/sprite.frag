#version 450
layout(early_fragment_tests) in;

layout(location=0) in vec2 uv0;
layout(location=1) flat in float a;
layout(location=2) flat in float ph;
layout(location=3) flat in float m;
layout(location=4) in float fx;

layout(binding=0) uniform sampler2D s;
layout(binding=1) uniform U{
    vec4 mo,w,iA,iB;
}u;

layout(location=0) out vec4 o;

float sat(float x){ return clamp(x, 0.0, 1.0); }

float h1(float p){
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float h2(vec2 p){
    vec3 q = fract(vec3(p.xyx) * .1031);
    q += dot(q, q.yzx + 33.33);
    return fract((q.x + q.y) * q.z);
}

float n(vec2 u){
    vec2 i = floor(u);
    vec2 f = fract(u);
    vec2 s = f * f * (3.0 - 2.0 * f);

    float a = h2(i);
    float b = h2(i + vec2(1.0, 0.0));
    float c = h2(i + vec2(0.0, 1.0));
    float d = h2(i + vec2(1.0, 1.0));

    return mix(mix(a, b, s.x), mix(c, d, s.x), s.y);
}

vec3 pal(float t, float k, float w){
    vec3 r  = vec3(.032, .074, .022);
    vec3 m1 = vec3(.102, .248, .070);
    vec3 m2 = vec3(.205, .392, .106);
    vec3 t1 = vec3(.390, .505, .158);

    float s0 = smoothstep(0.0, .34, t);
    float s1 = smoothstep(.24, .78, t);
    float s2 = smoothstep(.74, 1.0, t);

    vec3 c = mix(r,  m1, s0);
    c = mix(c, m2, s1);
    c = mix(c, t1 * (.97 + w * .08), s2);

    return c * mix(vec3(.93, .98, .93), vec3(1.05, 1.01, .92), k);
}

void acc(inout float A, inout vec3 C, float w, vec3 s){
    float k = sat(w) * (1.0 - A);
    C += s * k;
    A += k;
}

// تقريب سريع بدل pow(p,1.22)
// قريب بصريًا وأخف حسابيًا من pow
float pow122_fast(float p){
    float p2 = p * p;
    return mix(p, p2, 0.22);
}

void cell(
    vec2 uv, float dx, float id, float seed,
    float hMin, float hMax, float wMin, float wMax,
    float cut, float ms, float sf, float op, float tw,
    inout float A, inout vec3 C)
{
    float t  = u.mo.x;
    float st = u.mo.y;
    float sp = u.mo.z;

    float s = id * 19.17 + seed * 3.41;

    float lush = n(vec2(id * .23 + seed * .09, seed * .17));
    float occ  = mix(h1(s + 1.0), lush, .34);
    if(occ < cut) return;

    float r2 = h1(s + 2.0);
    float r3 = h1(s + 3.0);
    float r4 = h1(s + 4.0);

    float cx  = id + mix(.16, .84, r2);
    float lx  = dx - cx;
    float twi = mix(.18, .34, r3) * mix(.86, 1.14, lush);
    float rh  = mix(.11, .20, r4);

    float e = max(.0025, (fwidth(lx) + fwidth(uv.y)) * sf);

    float rm =
        (1.0 - smoothstep(twi, twi + e * 3.5, abs(lx))) *
        (1.0 - smoothstep(rh * .9, rh * 1.2, uv.y));

    float invRh2 = 1.0 / max(rh * 2.0, .001);
    vec3 rootCol = pal(min(.38, uv.y * invRh2), occ, tw) * mix(.7, .9, lush);

    acc(A, C, rm * op, rootCol);

    float windBase1 = t * .28 * sp + fx * 1.35;
    float windBase2 = t * .92 * sp + uv.y * 4.2;

    for(int i = 0; i < 2; ++i){
        float bs = s + float(i) * 13.7;

        float r5  = h1(bs + 5.0);
        float r6  = h1(bs + 6.0);
        float r7  = h1(bs + 7.0);
        float r8  = h1(bs + 8.0);
        float r9  = h1(bs + 9.0);
        float r10 = h1(bs + 10.0);
        float r11 = h1(bs + 11.0);
        float r12 = h1(bs + 12.0);

        float bh = mix(hMin, hMax, r5);
        if(uv.y > bh + e * 6.0) continue;

        float invBh = 1.0 / max(bh, .001);
        float p  = sat(uv.y * invBh);
        float p2 = p * p;

        float bw    = mix(wMin, wMax, r6);
        float lean  = (r7 - .5) * .13;
        float curv  = (r8 - .5) * .18;
        float stiff = mix(1.22, .8, r9);
        float phs   = r10 * 6.2831853;

        float off =
            ((float(i) + .5) / 2.0 - .5) * twi * 1.18 +
            (r11 - .5) * twi * .32;

        float wv =
            sin(windBase1 + phs) * .72 +
            sin(windBase2 + phs * 2.0) * .10;

        float bend = mix(.78, 1.22, p2) * p2;

        float ctr =
            off +
            lean * p +
            curv * (p * (1.0 - p * .46)) +
            wv * st * ms * bend / stiff;

        float th = bw * mix(1.06, .05, pow122_fast(p));
        th += bw * .14 *
              smoothstep(.08, .3, p) *
              (1.0 - smoothstep(.36, .7, p));

        float d = abs(lx - ctr);
        float bm = 1.0 - smoothstep(th, th + e * 1.8, d);
        bm *= 1.0 - smoothstep(bh - e * 7.0, bh + e * 4.0, uv.y);

        if(bm <= .0001) continue;

        float tint = mix(r12, lush, .4);
        vec3 col = pal(p, tint, tw) * mix(.74, 1.02, smoothstep(0.0, .88, p));

        acc(A, C, bm, col);
    }
}

void tufts(
    vec2 uv, float fx, float seed, float ds,
    float hMin, float hMax, float wMin, float wMax,
    float cut, float ms, float sf, float op, float tw,
    inout float A, inout vec3 C)
{
    float dx = fx * ds + seed;
    float c  = floor(dx);
    float fr = fract(dx);
    float nn = c + (fr > .5 ? 1.0 : -1.0);

    cell(uv, dx, c,  seed,      hMin, hMax, wMin, wMax, cut,       ms, sf, op,       tw, A, C);
    if(A > .985) return;
    cell(uv, dx, nn, seed + 1.7, hMin, hMax, wMin, wMax, cut + .01, ms, sf, op * .95, tw, A, C);
}

void main(){
    if(a <= .001) discard;

    if(m <= .5){
        vec4 t = texture(s, uv0);
        if(t.a < .01) discard;
        o = vec4(t.rgb, t.a * a);
        return;
    }

    vec2 uv = vec2(uv0.x, 1.0 - uv0.y);

    if(m > 1.5){
        float ns = n(vec2(fx * 4.0, uv.y * 3.0 + 11.0));
        vec3 soil = mix(vec3(.16, .1, .05), vec3(.3, .19, .1), uv.y);
        o = vec4(soil * (.92 + ns * .12), a);
        return;
    }

    float d    = smoothstep(.2, .78, a);
    float seed = ph * .71 + n(vec2(fx * .9 + 3.0, d * 4.0));
    float dens = n(vec2(fx * .82 + seed * .2, d * 2.8));
    float fog  = n(vec2(fx * 3.0 + seed, uv.y * 1.8 + 13.0));

    float A = 0.0;
    vec3  C = vec3(0.0);

    acc(
        A, C,
        (1.0 - smoothstep(.15 + fog * .05, .45, uv.y)) * (.09 + dens * .07),
        mix(vec3(.032, .074, .022), vec3(.078, .148, .048), .42 + fog * .14)
    );

    tufts(
        uv, fx, seed,
        mix(12.1, 6.8, d) + dens * 1.1,
        mix(.34, .56, d), mix(.64, .98, d),
        mix(.026, .04, d), mix(.054, .088, d),
        mix(.34, .18, d) - dens * .07,
        mix(.42, .84, d),
        mix(1.22, .92, d),
        mix(.16, .26, d),
        mix(.04, .12, d),
        A, C
    );

    if(A < .028) discard;

    C *= mix(.8, 1.0, smoothstep(0.0, .34, uv.y));
    o = vec4(C, A * a);
}
