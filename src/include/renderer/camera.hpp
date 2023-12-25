#ifndef _ENGINE__CAMERA_HPP
#define _ENGINE__CAMERA_HPP

// what even?
#include <engine/math.hpp>
#include <renderer/renderer.hpp>

struct camera {
    glm::vec3 pos;
    glm::mat4 view;
    glm::mat4 proj;
    float fov;
    float ar;
    float near;
    float far;
    glm::vec2 rect;

    float horizontal;
    float vertical;
    glm::vec3 lookdirection;
    glm::vec3 right;

    struct ORenderer::renderer *renderer;
};

void camera_update(struct camera *camera);
void camera_init(struct ORenderer::renderer *renderer);


#endif
