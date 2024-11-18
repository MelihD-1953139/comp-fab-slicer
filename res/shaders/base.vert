#version 410 core
layout (location = 0) in vec3 aPos;

out vec3 FragPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vec4 pos = vec4(aPos, 1.0);
    gl_Position = projection * view * model * pos; 
    FragPos = vec3(model * pos);
}