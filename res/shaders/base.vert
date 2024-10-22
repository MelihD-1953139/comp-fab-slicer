#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 viewPos;
out vec3 vertexNormal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0); 
    FragPos = vec3(model * vec4(aPos, 1.0));
    vertexNormal = mat3(transpose(inverse(model))) * aNormal;
    viewPos = vec3(view * model * vec4(aPos, 1.0));
}