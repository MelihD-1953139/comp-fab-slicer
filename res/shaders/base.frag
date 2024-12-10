#version 410 core

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
}