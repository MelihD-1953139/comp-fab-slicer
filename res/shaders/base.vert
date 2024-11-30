#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vec4 pos = vec4(aPos, 1.0);
    gl_Position = projection * view * model * pos; 
    FragPos = vec3(model * pos);
    Normal = mat3(transpose(inverse(model))) * aNormal;
}