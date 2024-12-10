#version 410 core
layout (location = 0) in dvec2 aPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vec4 pos = vec4(float(aPos.x),  0.0, float(aPos.y), 1.0);
    gl_Position = projection * view * model * pos; 
}