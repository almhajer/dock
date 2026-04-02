#version 450
layout(early_fragment_tests) in;

layout(location=0) in vec2 uv0;
layout(location=1) in float a;
layout(location=2) in float ph;
layout(location=3) in float m;
layout(location=4) in vec2 f;

layout(binding=0) uniform sampler2D s;
layout(binding=1) uniform U{
    vec4 mo,w,iA,iB;
}u;

layout(location=0) out vec4 o;

float h1(float p){p=fract(p*.1031);p*=p+33.33;p*=p+p;return fract(p);}
float h2(vec2 p){
    vec3 q=fract(vec3(p.xyx)*.1031);
    q+=dot(q,q.yzx+33.33);
    return fract((q.x+q.y)*q.z);
}

float n(vec2 u){
    vec2 i=floor(u),f=fract(u),s=f*f*(3.-2.*f);
    float a=h2(i),b=h2(i+vec2(1,0)),c=h2(i+vec2(0,1)),d=h2(i+1.);
    return mix(mix(a,b,s.x),mix(c,d,s.x),s.y);
}

vec3 pal(float t,float k,float w){
    vec3 r=vec3(.032,.074,.022),
         m1=vec3(.102,.248,.070),
         m2=vec3(.205,.392,.106),
         t1=vec3(.390,.505,.158);
    vec3 c=mix(r,m1,smoothstep(0.,.34,t));
    c=mix(c,m2,smoothstep(.24,.78,t));
    c=mix(c,t1*(.97+w*.08),smoothstep(.74,1.,t));
    return c*mix(vec3(.93,.98,.93),vec3(1.05,1.01,.92),k);
}

void acc(inout float a,inout vec3 c,float w,vec3 s){
    float k=clamp(w,0.,1.)*(1.-a);
    c+=s*k; a+=k;
}

void cell(
    vec2 uv,float dx,float id,float seed,
    float hMin,float hMax,float wMin,float wMax,
    float cut,float ms,float sf,float op,float tw,
    inout float A,inout vec3 C)
{
    float t=u.mo.x,st=u.mo.y,sp=u.mo.z;
    float s=id*19.17+seed*3.41;
    float lush=n(vec2(id*.23+seed*.09,seed*.17));
    float occ=mix(h1(s+1.),lush,.34);
    if(occ<cut)return;

    float cx=id+mix(.16,.84,h1(s+2.));
    float lx=dx-cx;
    float twi=mix(.18,.34,h1(s+3.))*mix(.86,1.14,lush);
    float rh=mix(.11,.20,h1(s+4.));
    float e=max(.0025,(fwidth(lx)+fwidth(uv.y))*sf);

    float rm=(1.-smoothstep(twi,twi+e*3.5,abs(lx)))*
             (1.-smoothstep(rh*.9,rh*1.2,uv.y));

    acc(A,C,rm*op,
        pal(min(.38,uv.y/max(rh*2.,.001)),occ,tw)*
        mix(.7,.9,lush));

    for(int i=0;i<2;i++){
        float bs=s+float(i)*13.7;
        float bh=mix(hMin,hMax,h1(bs+5.));
        if(uv.y>bh+e*6.)continue;

        float p=clamp(uv.y/max(bh,.001),0.,1.);
        float bw=mix(wMin,wMax,h1(bs+6.));
        float lean=(h1(bs+7.)-.5)*.13;
        float curv=(h1(bs+8.)-.5)*.18;
        float stiff=mix(1.22,.8,h1(bs+9.));
        float phs=h1(bs+10.)*6.2831853;

        float off=((float(i)+.5)/2.-.5)*twi*1.18+
                  (h1(bs+11.)-.5)*twi*.32;

        float wv=
            sin(t*.28*sp+f.x*1.35+phs)*.72+
            sin(t*.92*sp+phs*2.+uv.y*4.2)*.1;

        float ctr=off+
            lean*p+
            curv*(p*(1.-p*.46))+
            wv*st*ms*mix(.78,1.22,p*p)*p*p/stiff;

        float th=bw*mix(1.06,.05,pow(p,1.22));
        th+=bw*.14*
            smoothstep(.08,.3,p)*
            (1.-smoothstep(.36,.7,p));

        float d=abs(lx-ctr);
        float bm=1.-smoothstep(th,th+e*1.8,d);
        bm*=1.-smoothstep(bh-e*7.,bh+e*4.,uv.y);

        if(bm<=.0001)continue;

        float tint=mix(h1(bs+12.),lush,.4);
        vec3 col=pal(p,tint,tw)*mix(.74,1.02,smoothstep(0.,.88,p));

        acc(A,C,bm,col);
    }
}

void tufts(
    vec2 uv,vec2 f,float seed,float ds,
    float hMin,float hMax,float wMin,float wMax,
    float cut,float ms,float sf,float op,float tw,
    inout float A,inout vec3 C)
{
    float dx=f.x*ds+seed;
    float c=floor(dx),fr=fract(dx);
    float n=c+(fr>.5?1.:-1.);

    cell(uv,dx,c,seed,hMin,hMax,wMin,wMax,cut,ms,sf,op,tw,A,C);
    cell(uv,dx,n,seed+1.7,hMin,hMax,wMin,wMax,cut+.01,ms,sf,op*.95,tw,A,C);
}

void main(){
    if(m<=.5){
        vec4 t=texture(s,uv0);
        if(t.a<.01)discard;
        o=vec4(t.rgb,t.a*a);
        return;
    }

    vec2 uv=vec2(uv0.x,1.-uv0.y);

    if(m>1.5){
        float ns=n(vec2(f.x*4.,uv.y*3.+11.));
        vec3 soil=mix(vec3(.16,.1,.05),vec3(.3,.19,.1),uv.y);
        o=vec4(soil*(.92+ns*.12),a);
        return;
    }

    float d=smoothstep(.2,.78,a);
    float seed=ph*.71+n(vec2(f.x*.9+3.,d*4.));
    float dens=n(vec2(f.x*.82+seed*.2,d*2.8));
    float fog=n(vec2(f.x*3.+seed,uv.y*1.8+13.));

    float A=0.;
    vec3 C=vec3(0.);

    acc(A,C,
        (1.-smoothstep(.15+fog*.05,.45,uv.y))*(.09+dens*.07),
        mix(vec3(.032,.074,.022),vec3(.078,.148,.048),.42+fog*.14));

    tufts(
        uv,f,seed,
        mix(12.1,6.8,d)+dens*1.1,
        mix(.34,.56,d),mix(.64,.98,d),
        mix(.026,.04,d),mix(.054,.088,d),
        mix(.34,.18,d)-dens*.07,
        mix(.42,.84,d),
        mix(1.22,.92,d),
        mix(.16,.26,d),
        mix(.04,.12,d),
        A,C);

    if(A<.028)discard;

    C*=mix(.8,1.,smoothstep(0.,.34,uv.y));
    o=vec4(C,A*a);
}
