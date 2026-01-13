#version 410 core

in vec2 v_Screen;
layout (location = 0) out vec4 FragColor;

uniform mat3 u_CameraRotation; // extracted from VIEW matrix
uniform vec2 u_ViewportPx;

float sdSegment(vec2 p, vec2 a, vec2 b)
{
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main()
{
    // Screen pixel coords
    vec2 px = (v_Screen * 0.5 + 0.5) * u_ViewportPx;

    // Top-right anchor
    vec2 origin = u_ViewportPx - vec2(80.0, 80.0);
    vec2 p = px - origin;

    float len = 55.0;
    float thick = 2.0;
    float aa = 1.0;

    // === CAMERA-SPACE AXES (standard) ===
    vec3 X3 = normalize(u_CameraRotation[0]); // camera right
    vec3 Y3 = normalize(u_CameraRotation[1]); // camera up
    vec3 Z3 = normalize(u_CameraRotation[2]); // camera forward

    // Project into screen space (Y flipped ONCE)
    vec2 X = normalize(vec2(X3.x, -X3.y));
    vec2 Y = normalize(vec2(Y3.x, -Y3.y));
    vec2 Z = normalize(vec2(Z3.x, -Z3.y));

    vec2 a = vec2(0.0);
    vec2 bx = X * len;
    vec2 by = Y * len;
    vec2 bz = Z * len;

    float dx = sdSegment(p, a, bx);
    float dy = sdSegment(p, a, by);
    float dz = sdSegment(p, a, bz);

    float mx = 1.0 - smoothstep(thick - aa, thick + aa, dx);
    float my = 1.0 - smoothstep(thick - aa, thick + aa, dy);
    float mz = 1.0 - smoothstep(thick - aa, thick + aa, dz);

    if (max(mx, max(my, mz)) < 0.01)
    discard;

    // Standard axis colors
    vec3 col =
    mx * vec3(1,0,0) +       // X = red
    my * vec3(1,1,0) +       // Y = yellow
    mz * vec3(0,1,0);        // Z = green

    FragColor = vec4(col, 1.0);
}
