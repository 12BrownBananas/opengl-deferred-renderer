#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat3 normalMatrix;

void main() {
	vec4 worldPos = model * vec4(aPosition, 1.0);
	vs_out.FragPos = worldPos.xyz;

	vs_out.Normal = normalize(normalMatrix*aNormal);
	vs_out.TexCoords = aTexCoords;
	gl_Position = projection * view * worldPos;
}