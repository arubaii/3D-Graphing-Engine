#version 300 es
precision highp float;
precision highp sampler3D;

in vec2 v_UV;
out vec4 FragColor;

uniform mat4 u_InvViewProj;
uniform vec3 u_CamPos;

uniform vec3 u_BoxMin;
uniform vec3 u_BoxMax;

uniform sampler3D u_FieldTex;   // R32F volume (or R16F)
uniform float u_Iso;            // usually 0
uniform float u_StepScale;      // tweak for speed/quality (e.g. 1.0)

uniform vec4 u_Color;           // surface color
uniform vec3 u_LightPos;
uniform vec4 u_LightColor;

uniform ivec3 u_FieldDim;
uniform float u_K;

uniform float u_GridScale;
uniform float u_GridWidth;
uniform vec3  u_GridColor;
uniform float u_GridAlpha;


// Color profiles
uniform bool usePastel;
uniform bool useNormal;
uniform bool userColor;



float sampleField(vec3 p)
{
    vec3 bmin = u_BoxMin * u_K;
    vec3 bmax = u_BoxMax * u_K;
    vec3 uvw = (p - bmin) / (bmax - bmin);
    return texture(u_FieldTex, uvw).r - u_Iso;
}

vec3 fieldNormal(vec3 pWorld, float baseDt)
{
    vec3 sizeW = (u_BoxMax - u_BoxMin);
    float eps = baseDt * 1.0; // 1 voxel

    float fx1 = sampleField(pWorld + vec3(eps, 0.0, 0.0));
    float fx0 = sampleField(pWorld - vec3(eps, 0.0, 0.0));
    float fy1 = sampleField(pWorld + vec3(0.0, eps, 0.0));
    float fy0 = sampleField(pWorld - vec3(0.0, eps, 0.0));
    float fz1 = sampleField(pWorld + vec3(0.0, 0.0, eps));
    float fz0 = sampleField(pWorld - vec3(0.0, 0.0, eps));

    vec3 g = vec3(fx1 - fx0, fy1 - fy0, fz1 - fz0);
    float l = max(length(g), 1e-8);
    return g / l;
}

bool intersectBox(vec3 ro, vec3 rd, vec3 bmin, vec3 bmax, out float t0, out float t1)
{
    vec3 invD = 1.0 / rd;
    vec3 tbot = (bmin - ro) * invD;
    vec3 ttop = (bmax - ro) * invD;
    vec3 tminv = min(tbot, ttop);
    vec3 tmaxv = max(tbot, ttop);

    t0 = max(max(tminv.x, tminv.y), tminv.z);
    t1 = min(min(tmaxv.x, tmaxv.y), tmaxv.z);
    return t1 >= max(t0, 0.0);
}

vec4 Coloring(vec3 N)
{
    vec4 col;
    if (useNormal)
    {
        vec3 n01 = normalize(N) * 0.5 + 0.5;
        col = vec4(n01, u_Color.a);
    }

    else if (usePastel)
    {
        float s = clamp(0.5 + 0.5 * N.y, 0.0, 1.0);
        float u  = clamp(0.5 + 0.5 * N.x, 0.0, 1.0);

        vec3 coolTop  = vec3(0.52, 0.76, 1.00);
        vec3 warmBot  = vec3(1.00, 0.60, 0.62);
        vec3 lavender = vec3(0.72, 0.64, 1.00);

        vec3 base = mix(warmBot, coolTop, s);
        base = mix(base, lavender, 0.35 * (1.0 - abs(2.0 * u - 1.0)));
        base = mix(base, vec3(1.0), 0.08);

        col = vec4(base, u_Color.a);
    }
    else
    {
        col = u_Color;
    }

    return col;
}

float sampleFieldLod(vec3 p, float lod)
{
    vec3 bmin = u_BoxMin * u_K;
    vec3 bmax = u_BoxMax * u_K;
    vec3 uvw = (p - bmin) / (bmax - bmin);
    return textureLod(u_FieldTex, uvw, lod).r - u_Iso;
}

float gridLines2D(vec2 uvWorld, float scale, float widthMul)
{
    vec2 p = uvWorld * scale;
    vec2 a = abs(fract(p) - 0.5);
    vec2 fw = fwidth(p);
    float lx = 1.0 - smoothstep(0.0, widthMul * fw.x, a.x);
    float ly = 1.0 - smoothstep(0.0, widthMul * fw.y, a.y);
    return max(lx, ly);
}

float gridTriplanar(vec3 worldPos, vec3 nWorld)
{
    vec3 n = normalize(nWorld);
    vec3 w = pow(abs(n), vec3(4.0));
    w /= max(w.x + w.y + w.z, 1e-6);

    float gx = gridLines2D(worldPos.yz, u_GridScale, u_GridWidth);
    float gy = gridLines2D(worldPos.xz, u_GridScale, u_GridWidth);
    float gz = gridLines2D(worldPos.xy, u_GridScale, u_GridWidth);

    return gx * w.x + gy * w.y + gz * w.z;
}

vec4 RaymarchSample(vec3 ro, vec3 rd)
{
    float tEnter, tExit;
    if (!intersectBox(ro, rd, u_BoxMin, u_BoxMax, tEnter, tExit))
    return vec4(0.0);

    vec3 boxSize = (u_BoxMax - u_BoxMin);
    vec3 texelW  = boxSize / vec3(u_FieldDim);
    float baseDt = min(texelW.x, min(texelW.y, texelW.z));
    float dt = baseDt * u_StepScale;     // u_StepScale ~ 0.75..2.0
    dt = max(dt, 1e-5);


    vec3 bminK = u_BoxMin * u_K;
    vec3 bmaxK = u_BoxMax * u_K;
    vec3 spanK = (bmaxK - bminK);

    float dtTex = dt / max(min(spanK.x, min(spanK.y, spanK.z)), 1e-6);
    float lod = max(0.0, log2(dtTex * float(max(max(u_FieldDim.x, u_FieldDim.y), u_FieldDim.z))));

    float tLen = (tExit - tEnter);
    int maxSteps = int(clamp(ceil(tLen / dt), 1.0, 2048.0));

    float t = tEnter;
    float fPrev = sampleFieldLod(ro + rd * t, lod);

    bool hit = false;
    float tHit = t;

    for (int i = 0; i < 2048; ++i)
    {
        if (i >= maxSteps) break;

        t += dt;
        if (t > tExit) break;

        vec3 p = ro + rd * t;
        float f = sampleFieldLod(p, lod);

        if ((fPrev <= 0.0 && f >= 0.0) || (fPrev >= 0.0 && f <= 0.0))
        {
            float a = t - dt;
            float b = t;
            float fa = fPrev;
            float fb = f;

            for (int it = 0; it < 10; ++it)
            {
                float m = 0.5 * (a + b);
                float fm = sampleFieldLod(ro + rd * m, lod);
                bool s = (fa <= 0.0 && fm >= 0.0) || (fa >= 0.0 && fm <= 0.0);
                if (s) { b = m; fb = fm; }
                else   { a = m; fa = fm; }
            }

            tHit = 0.5 * (a + b);
            hit = true;
            break;
        }

        fPrev = f;
    }

    if (!hit)
    return vec4(0.0);

    vec3 pHit = ro + rd * tHit;
    vec3 N = normalize(fieldNormal(pHit, baseDt));
    vec3 V = normalize(u_CamPos - pHit);

    // Interior normals
    if (dot(N, V) < 0.0)
        N = -N;

    vec3 L = normalize(u_LightPos - pHit);


    vec4 baseColor = Coloring(N);

    float ambient = 0.12;

    float ndl = dot(N, L);
    float diff = smoothstep(-0.15, 0.85, ndl);

    vec3 H = normalize(L + V);
    float specular = pow(max(dot(N, H), 0.0), 96.0) * 0.18;

    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0) * 0.20;

    float shadow = 0.65 + 0.35 * smoothstep(-0.2, 0.6, ndl);

    vec3 lc = u_LightColor.rgb;

    vec4 color =
    baseColor * (ambient + diff * shadow)
    + vec4(lc * specular, 0.0)
    + rim * baseColor;

    float g = gridTriplanar(pHit / max(u_K, 1e-6), N);
    color.rgb = mix(color.rgb, u_GridColor, g * u_GridAlpha);

    vec3 rgb = color.rgb;
    rgb = rgb / (rgb + vec3(1.0));
    rgb = pow(rgb, vec3(1.0 / 2.2));

    return vec4(rgb, 1.0);
}

void main()
{
    vec2 duvdx = dFdx(v_UV);
    vec2 duvdy = dFdy(v_UV);

    const vec2 rgss[4] = vec2[4](
    vec2(-3.0,  1.0) / 8.0,
    vec2( 1.0,  3.0) / 8.0,
    vec2( 3.0, -1.0) / 8.0,
    vec2(-1.0, -3.0) / 8.0
    );

    vec4 acc = vec4(0.0);

    for (int i = 0; i < 4; ++i)
    {
        vec2 uv  = v_UV + rgss[i].x * duvdx + rgss[i].y * duvdy;
        vec2 ndc = uv * 2.0 - 1.0;

        vec4 nearH = u_InvViewProj * vec4(ndc, -1.0, 1.0);
        vec4 farH  = u_InvViewProj * vec4(ndc,  1.0, 1.0);
        vec3 pNear = nearH.xyz / nearH.w;
        vec3 pFar  = farH.xyz  / farH.w;

        vec3 rd = normalize(pFar - u_CamPos);
        acc += RaymarchSample(u_CamPos, rd);
    }

    float exposure = 1.25;
    FragColor = acc * 0.25 * exposure;
}