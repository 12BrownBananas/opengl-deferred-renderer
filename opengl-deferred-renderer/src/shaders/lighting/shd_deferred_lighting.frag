#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D ssao;
uniform bool useSSAO;

struct Light {
    vec3 Position;
    vec3 Color;
    float Linear;
    float Quadratic;
    float Radius;
};
const int NR_LIGHTS = 32;
uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;

uniform vec3 ambientLightColor;

void main()
{             
    // retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float AmbientOcclusion = 1.0;
    if (useSSAO) AmbientOcclusion = texture(ssao, TexCoords).r;
    
    // then calculate lighting as usual
    vec3 ambient = ambientLightColor*vec3(Diffuse*AmbientOcclusion);
    vec3 lighting  = ambient;
    vec3 viewDir  = normalize(- FragPos);

    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        float dist = length(lights[i].Position - FragPos);
        if (dist < lights[i].Radius) { //NOTE: this isn't optimal. You're going to want to render spheres instead.
            // diffuse
            vec3 lightDir = normalize(lights[i].Position - FragPos);
            vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights[i].Color;
            // specular
            vec3 halfwayDir = normalize(lightDir + viewDir);  
            float spec = pow(max(dot(Normal, halfwayDir), 0.0), 8.0);
            vec3 specular = lights[i].Color * spec;
            // attenuation

            float attenuation = 1.0 / (1.0 + lights[i].Linear * dist + lights[i].Quadratic * dist * dist);
            diffuse *= attenuation;
            specular *= attenuation;
            lighting += diffuse + specular;
        }
    }
    FragColor = vec4(lighting, 1.0);
}  