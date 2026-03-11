#version 300 es
precision highp float;

layout (location = 0) in vec3 a_Position;
layout (location = 3) in vec3 a_Color;

uniform mat4 u_MVP;
out vec3 v_Color;

void main()
{
    v_Color = a_Color;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}