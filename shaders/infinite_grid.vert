#version 410 core

out vec3 WorldPos;

uniform mat4 Projection;
uniform mat4 View;
uniform vec3 CameraWorldPos;
uniform float gGridSize;
uniform float GridHeight;

const vec3 Pos[4] = vec3[4](
vec3(-1, 0, -1),
vec3( 1, 0, -1),
vec3( 1, 0,  1),
vec3(-1, 0,  1)
);

const uint Indices[6] = uint[6](0,1,2, 0,2,3);

void main()
{
    vec3 p = Pos[Indices[gl_VertexID]] * gGridSize;

    // Center grid on camera in XZ
    p.x += CameraWorldPos.x;
    p.y = GridHeight;
    p.z += CameraWorldPos.z;

    WorldPos = p;
    gl_Position = Projection * View * vec4(p, 1.0);
}

