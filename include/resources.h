#pragma once

#include <cstdlib>

static const char *baseVertexShader = R"(#version 410 core
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
})";
static const char *sliceVertexShader = R"(#version 410 core
layout (location = 0) in dvec2 aPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vec4 pos = vec4(float(aPos.x),  0.0, float(aPos.y), 1.0);
    gl_Position = projection * view * model * pos; 
})";
static const char *baseFragmentShader = R"(#version 410 core

in vec3 FragPos;
in vec3 Normal;

uniform vec3 color;
uniform vec3 lightPos;
uniform bool useShading;

out vec4 FragColor;


void main()
{
	if (useShading){
		// ambient
		float ambientStrength = 0.1;
		vec3 ambient = ambientStrength * color;

		// diffuse
		vec3 norm = normalize(Normal);
		vec3 lightDir = normalize(lightPos - FragPos);
		float diff = max(dot(norm, lightDir), 0.0);
		vec3 diffuse = diff * color;

		vec3 result = (ambient + diffuse);
		FragColor = vec4(result, 1.0);
	} else {
		FragColor = vec4(color, 1.0);
	}
})";
static const char *sliceFragmentShader = R"(#version 410 core

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
