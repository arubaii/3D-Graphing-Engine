#version 410 core

in vec3 WorldPos;

layout (location = 0) out vec4 FragColor;

uniform float gGridCellSize  = 2.0;
uniform float gGridMinPixelsBetweenCells = 2.0;
uniform vec4 gGridColorThin  = vec4(0.35, 0.35, 0.35, 1.0);
uniform vec4 gGridColorThick = vec4(0.6, 0.6, 0.6, 1.0);
uniform vec4 xAxisColor = vec4(0.4, 0.7, 0.4, 1.0); // Green
uniform vec4 zAxisColor = vec4(0.7, 0.3, 0.3, 1.0); // Red
uniform vec3 CameraWorldPos;
uniform float gGridSize = 400;

float log10(float x)
{   // log10 N/A in 410
    return log(x) / log(10.0);
}

void main(){

    vec2 dvx = vec2(dFdx(WorldPos.x), dFdy(WorldPos.x));
    vec2 dvy = vec2(dFdx(WorldPos.z), dFdy(WorldPos.z));

    float lx = length(dvx);
    float ly = length(dvy);

    vec2 dudv = vec2(lx, ly);
    float l = length(dudv);
    float LOD = max(0.0, log10(l * gGridMinPixelsBetweenCells / gGridCellSize) + 1.0);

    float GridCellSizeLod0 = gGridCellSize * pow(10.0, floor(LOD));
    float GridCellSizeLod1 = GridCellSizeLod0 * 10.0;
    float GridCellSizeLod2 = GridCellSizeLod1 * 10.0;

    dudv *= 4.0;

    // Axis line detection - use appropriate derivative for each axis
    float xAxisWidth = dudv.y * 0.75; // X-axis runs along X, varies in Z direction
    float zAxisWidth = dudv.x * 0.75; // Z-axis runs along Z, varies in X direction

    float xAxisMask = 1.0 - clamp(abs(WorldPos.z) / xAxisWidth, 0.0, 1.0);
    float zAxisMask = 1.0 - clamp(abs(WorldPos.x) / zAxisWidth, 0.0, 1.0);

    vec4 Color;

    // Check if we're on an axis first
    if (xAxisMask > 0.0) {
        Color = xAxisColor;
        Color.a = xAxisMask;
    }
    else if (zAxisMask > 0.0) {
        Color = zAxisColor;
        Color.a = zAxisMask;
    }
    else // Only draw grid if we're not on an axis
    {

        vec2 mod_div_dudv = mod(WorldPos.xz, GridCellSizeLod0) / dudv;
        // Level of Detail 0
        float Lod0a = max(
        (1.0 - abs(clamp(mod_div_dudv.x, 0.0, 1.0) * 2.0 - 1.0)),
        (1.0 - abs(clamp(mod_div_dudv.y, 0.0, 1.0) * 2.0 - 1.0))
        );

        mod_div_dudv = mod(WorldPos.xz, GridCellSizeLod1) / dudv;
        // Level of Detail 1
        float Lod1a = max(
        (1.0 - abs(clamp(mod_div_dudv.x, 0.0, 1.0) * 2.0 - 1.0)),
        (1.0 - abs(clamp(mod_div_dudv.y, 0.0, 1.0) * 2.0 - 1.0))
        );

        mod_div_dudv = mod(WorldPos.xz, GridCellSizeLod2) / dudv;
        // Level of Detail 2
        float Lod2a = max(
        (1.0 - abs(clamp(mod_div_dudv.x, 0.0, 1.0) * 2.0 - 1.0)),
        (1.0 - abs(clamp(mod_div_dudv.y, 0.0, 1.0) * 2.0 - 1.0))
        );

        float LOD_fade = fract(LOD);

        if (Lod2a > 0.0)
        {
            Color = gGridColorThick;
            Color.a *= Lod2a;
        }
        else
        {
            if (Lod1a > 0.0)
            {
                Color = mix(gGridColorThick, gGridColorThin, LOD_fade);
                Color.a *= Lod1a;
            }
            else
            {
                Color = gGridColorThin;
                Color.a *= (Lod0a * (1 - LOD_fade));
            }
        }
    }

    float OpacityFalloff = 1.0 - clamp(length(WorldPos.xz - CameraWorldPos.xz) / gGridSize, 0.0, 1.0);

    Color.a *= OpacityFalloff;

    FragColor = Color;

}