#version 300 es
precision highp float;

in vec3 Normal;
in vec3 FragPos;
in vec3 vWorldPos;

out vec4 FragColor;

uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform vec4 lightColor;

uniform vec4 u_Color;

uniform vec3 u_BoxMin;
uniform vec3 u_BoxMax;
uniform float u_ContentScale;

uniform float u_GridScale;   // lines per world-unit
uniform float u_GridWidth;   // thickness multiplier (screen-stable via fwidth)
uniform vec3  u_GridColor;   // grid line color
uniform float u_GridAlpha;   // blend strength [0..1]



// Color profil
uniform bool usePastel;
uniform bool useNormal;
uniform bool userColor;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

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



float gridLines2D(vec2 uvWorld, float scale, float widthMul)
{
    vec2 p = uvWorld * scale;
    vec2 a = abs(fract(p) - 0.5);
    vec2 fw = fwidth(p);
    float lx = 1.0 - smoothstep(0.0, widthMul * fw.x, a.x);
    float ly = 1.0 - smoothstep(0.0, widthMul * fw.y, a.y);
    return max(lx, ly);
}


// triplanar grid blended by normal (world-space)
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

void main()
{
    vec3 d0 = vWorldPos - u_BoxMin;
    vec3 d1 = u_BoxMax  - vWorldPos;
    vec3 fw = fwidth(vWorldPos);

    float fx = min(smoothstep(-fw.x, fw.x, d0.x), smoothstep(-fw.x, fw.x, d1.x));
    float fy = min(smoothstep(-fw.y, fw.y, d0.y), smoothstep(-fw.y, fw.y, d1.y));
    float fz = min(smoothstep(-fw.z, fw.z, d0.z), smoothstep(-fw.z, fw.z, d1.z));
    float boxMask = fx * fy * fz;

    if (boxMask <= 0.0) discard;

    vec3 N = normalize(Normal);
    vec3 V = normalize(cameraPos - FragPos);
    vec3 L = normalize(lightPos - FragPos);

    // Interior normals
    if (dot(N, V) < 0.0)
    N = -N;

    vec4 baseColor = Coloring(N);


    float ambient = 0.12;

    float ndl = dot(N, L);
    float diff = smoothstep(-0.15, 0.85, ndl);

    vec3 H = normalize(L + V);
    float specular = pow(max(dot(N, H), 0.0), 96.0) * 0.18;

    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0) * 0.20 * boxMask;

    float shadow = 0.65 + 0.35 * smoothstep(-0.2, 0.6, ndl);

    vec3 lc = lightColor.rgb;

    vec4 color =
    baseColor * (ambient + diff * shadow)
    + vec4(lc * specular, 0.0)
    + rim * baseColor;

    float g = gridTriplanar(vWorldPos / max(u_ContentScale, 1e-6), N);
    color.rgb = mix(color.rgb, u_GridColor, g * u_GridAlpha);

    vec3 rgb = color.rgb;
    rgb = rgb / (rgb + vec3(1.0));
    rgb = pow(rgb, vec3(1.0/2.2));

    float exposure = 1.2;
    FragColor = vec4(rgb * boxMask, color.a * boxMask) * exposure;
}