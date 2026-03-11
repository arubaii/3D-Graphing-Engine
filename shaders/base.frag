#version 300 es
precision highp float;

in vec3 v_Color;
out vec4 FragColor;

void main()
{
    FragColor = vec4(v_Color, 1.0);
}