#version 300 es
precision highp float;

out vec3 WorldPos;

uniform mat4 Projection;
uniform mat4 View;
uniform vec3 CameraWorldPos;
uniform float gGridSize;
uniform float GridHeight;

const vec3 Pos[4] = vec3[4](
vec3(-1.0, 0.0, -1.0),
vec3( 1.0, 0.0, -1.0),
vec3( 1.0, 0.0,  1.0),
vec3(-1.0, 0.0,  1.0)
);

const uint Indices[6] = uint[6](0u, 1u, 2u, 0u, 2u, 3u);

void main()
{
    vec3 p = Pos[Indices[uint(gl_VertexID)]] * gGridSize;

    p.x += CameraWorldPos.x;
    p.y = GridHeight;
    p.z += CameraWorldPos.z;

    WorldPos = p;
    gl_Position = Projection * View * vec4(p, 1.0);
}