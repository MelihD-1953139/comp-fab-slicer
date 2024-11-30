#pragma once

#include <cstdlib>

static const char *vertexShader = R"(#version 410 core
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
})";
static const char *fragmentShader = R"(#version 410 core

in vec3 FragPos;
uniform vec3 color;

out vec4 FragColor;

void main()
{
	FragColor = vec4(color, 1.0);
})";
static const char *planeOBJ = R"(v 0 0 0
v 1 0 0
v 0 1 0
v 1 1 0

f 1 2 4
f 1 4 3)";
static const size_t planeOBJSize = atoi("48");
