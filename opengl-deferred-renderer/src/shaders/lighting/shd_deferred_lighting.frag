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
    float FarPlane;
    samplerCube Cubemap;
};
const int NR_LIGHTS = 16;
uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;

uniform vec3 ambientLightColor;

float shadow_calculation(vec3 fragPos, Light l)
{
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - l.Position;
    // ise the fragment to light vector to sample from the depth map    
    float closestDepth = texture(l.Cubemap, fragToLight).r;
    // it is currently in linear range between [0,1], let's re-transform it back to original depth value
    closestDepth *= l.FarPlane;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // test for shadows
    float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
    float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;        
    
    return shadow;
}

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
            float shadow = shadow_calculation(FragPos, lights[i]);
            diffuse *= attenuation;
            specular *= attenuation;
            lighting += (1.0 - shadow)*(diffuse + specular);
        }
    }
    FragColor = vec4(lighting, 1.0);
}  