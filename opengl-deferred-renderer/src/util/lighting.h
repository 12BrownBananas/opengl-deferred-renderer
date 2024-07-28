#ifndef LIGHTING_H
#define LIGHTING_H

#include <glm/glm.hpp>
#include <vector>
#include <render_util.h>

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
    PointLight(glm::vec3 position, glm::vec3 lightColor, glm::vec2 cubemapDimensions);
    const float nearPlane = 0.25f;
    const float farPlane = 25.0f;
protected:
    glm::vec2 cubemapDimensions;
};

#endif