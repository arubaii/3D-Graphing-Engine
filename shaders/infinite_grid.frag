#version 300 es
precision highp float;

in vec3 WorldPos;
out vec4 FragColor;

uniform vec3 CameraWorldPos;
uniform vec3 u_BoxMin;
uniform vec3 u_BoxMax;
uniform float u_ContentScale;
uniform float gGridSize;
uniform vec3 greyScale;


float saturate(float x) { return clamp(x, 0.0, 1.0); }
float log10f(float x) { return log(x) / log(10.0); }

float gridLineAA(vec2 p, float cell, float thicknessPx)
{
    vec2 q = p / cell;
    vec2 fw = max(fwidth(q), vec2(1e-6));
    vec2 distToLine = 0.5 - abs(fract(q) - 0.5);
    vec2 distPx = distToLine / fw;
    float d = min(distPx.x, distPx.y);
    float a = 1.0 - smoothstep(thicknessPx, thicknessPx + 1.0, d);
    return saturate(a);
}

void main()
{
    vec2 p = WorldPos.xz;

    // hard clip (prevents the angle-dependent "inside gradient" problem)
    if (p.x < u_BoxMin.x || p.x > u_BoxMax.x || p.y < u_BoxMin.z || p.y > u_BoxMax.z)
    discard;

    vec4 thinCol  = vec4(0.35, 0.35, 0.35, 1.0);
    vec4 thickCol = vec4(0.60, 0.60, 0.60, 1.0);

    vec4 xAxisColor = vec4(0.40, 0.70, 0.40, 1.0);
    vec4 zAxisColor = vec4(0.70, 0.30, 0.30, 1.0);

    float gGridCellSize = 2.0;
    float gGridMinPixelsBetweenCells = 2.0;

    float boxSpan = max(u_BoxMax.x - u_BoxMin.x, u_BoxMax.z - u_BoxMin.z);

    float cell = gGridCellSize * max(u_ContentScale, 1e-8);

    float maxCells = 1024.0;
    float minCells = 8.0;

    float minCell = boxSpan / maxCells;
    float maxCell = boxSpan / minCells;

    cell = clamp(cell, minCell, maxCell);

    vec2 dvx = vec2(dFdx(WorldPos.x), dFdy(WorldPos.x));
    vec2 dvz = vec2(dFdx(WorldPos.z), dFdy(WorldPos.z));

    float lx = length(dvx);
    float lz = length(dvz);

    vec2 dudv = max(vec2(lx, lz), vec2(1e-6));
    float l = length(dudv);

    float LOD = log10f(l * gGridMinPixelsBetweenCells / cell);
    LOD = max(0.0, LOD);

    float cell0 = cell * pow(10.0, floor(LOD));
    float cell1 = cell0 * 10.0;
    float cell2 = cell1 * 10.0;

    float fade = fract(LOD);

    float thinPx  = 0.25;
    float thickPx = 0.45;

    float g0 = gridLineAA(p, cell0, thinPx);
    float g1 = gridLineAA(p, cell1, thinPx);
    float g2 = gridLineAA(p, cell2, thickPx);

    float thinA  = mix(g1, g0, fade);
    float thickA = g2;

    vec3 rgb = thinCol.rgb;
    float a  = thinA;

    rgb = mix(rgb, thickCol.rgb, thickA);
    a   = max(a, thickA);

    float axisPx = 1.0;
    float wz = max(fwidth(WorldPos.z), 1e-6);
    float wx = max(fwidth(WorldPos.x), 1e-6);

    float xAxisMask = 1.0 - smoothstep(0.0, axisPx * wz, abs(WorldPos.z));
    float zAxisMask = 1.0 - smoothstep(0.0, axisPx * wx, abs(WorldPos.x));

    rgb = mix(rgb, xAxisColor.rgb, xAxisMask);
    a   = max(a, xAxisMask);

    rgb = mix(rgb, zAxisColor.rgb, zAxisMask);
    a   = max(a, zAxisMask);

//    float dist = length(WorldPos.xz - CameraWorldPos.xz);
//    float falloff = 1.0 - clamp(dist / gGridSize, 0.0, 1.0);
//    a *= falloff;

    // stable 1px-ish AA border using fwidth(distanceToEdge)
    float dx = min(p.x - u_BoxMin.x, u_BoxMax.x - p.x);
    float dz = min(p.y - u_BoxMin.z, u_BoxMax.z - p.y);
    float d  = min(dx, dz);               // world units to nearest edge (inside)

    float w  = max(fwidth(d), 1e-6);      // world units per pixel for this scalar
    float borderPx = 1.25;                // tweak: 1.0..2.0
    float border = 1.0 - smoothstep(0.0, borderPx * w, d);

    rgb = mix(rgb, greyScale, border);
    a   = max(a, border);

    FragColor = vec4(rgb, a);
}