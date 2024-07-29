#ifndef LIGHTING_H
#define LIGHTING_H

#include <glm/glm.hpp>
#include <vector>
#include <render_util.h>

struct LightAttenuationInfo {
    float linear = 0.7f;
    float quadratic = 1.8f;
    float constant = 1.0f;
};

class Light {
public:
    glm::vec3 position;
    glm::vec3 color;
    Light(glm::vec3 position, glm::vec3 lightColor);
};
class PointLight : public Light {
public:
    std::vector<glm::mat4> shadowTransforms;
    glm::mat4 shadowProj;
    unsigned int depthMapFBO;
    unsigned int depthCubemap;
    LightAttenuationInfo attenuationInfo;
    PointLight(glm::vec3 position, glm::vec3 lightColor, glm::vec2 cubemapDimensions, LightAttenuationInfo attenuationInfo);
    const float nearPlane = 0.25f;
    const float farPlane = 25.0f;
protected:
    glm::vec2 cubemapDimensions;
};

#endif