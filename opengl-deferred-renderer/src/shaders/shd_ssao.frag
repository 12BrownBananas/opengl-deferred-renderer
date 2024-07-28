#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;
uniform vec2 noiseScale;

uniform vec3 samples[64];

uniform mat4 projection;
uniform mat4 view;

uniform int kernelSize;
uniform float radius;
uniform float bias;

void main() {
	vec3 fragPos = texture(gPosition, TexCoords).xyz;
	vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
	vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);

	vec3 tangent = normalize(randomVec-normal*dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	float occlusion = 0.0;
	for (int i = 0; i < kernelSize; ++i) {
		vec3 samplePos = TBN*samples[i]; //transforms sample[i] from tangent space to view space
		samplePos = fragPos+samplePos*radius;

		vec4 offset = vec4(samplePos, 1.0); //offset is samplePos transformed to clip space
		offset = projection*view*offset;
		offset.xyz /= offset.w; //perspective divide performed to convert clip space to normalized device coordinates
		offset.xyz = offset.xyz * 0.5+0.5;

		float sampleDepth = texture(gPosition, offset.xy).z;
		float rangeCheck = smoothstep(0.0, 1.0, radius/abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
	}
	occlusion = 1.0-(occlusion/kernelSize);
	FragColor = occlusion;
}