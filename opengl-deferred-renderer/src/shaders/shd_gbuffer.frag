#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
} fs_in;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main() {
	//first texture gets position
	gPosition = fs_in.FragPos;

	//second texture gets normal
	gNormal = fs_in.Normal;

	
	//third texture gets a combination of albedo and specular
	//albedo goes into the rgb
	gAlbedoSpec.rgb = texture(texture_diffuse1, fs_in.TexCoords).rgb;
	//specular goes into the alpha
	gAlbedoSpec.a = texture(texture_specular1, fs_in.TexCoords).r;
}