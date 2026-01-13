#version 410 core

layout (location = 0) in vec3 a_Position;

out vec2 v_Screen;

void main()
{
    // a_Position.xy is already in NDC [-1,1]
    v_Screen = a_Position.xy;
    gl_Position = vec4(a_Position, 1.0);
}
