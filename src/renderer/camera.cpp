#include <renderer/camera.hpp>
#include <engine/matrix.hpp>

void camera_update(struct camera *camera) {
    camera->vertical = glm::clamp(camera->vertical, glm::radians(-90.0f), glm::radians(90.0f));
    camera->horizontal = fmodf(camera->horizontal, glm::radians(360.0f));

    camera->lookdirection = glm::vec3(glm::cos(camera->vertical) * glm::sin(camera->horizontal), glm::sin(camera->vertical), glm::cos(camera->vertical) * glm::cos(camera->horizontal));
    camera->right = glm::vec3(glm::sin(camera->horizontal - M_PI_2), 0.0f, glm::cos(camera->horizontal - M_PI_2));

    camera->ar = camera->rect.x / camera->rect.y;

    // mtx_lookat((float *)&camera->view, camera->pos, glms_vec3_add(camera->pos, camera->lookdirection), glms_vec3_cross(camera->right, camera->lookdirection));
    // camera->view = glms_lookat(camera->pos, glms_vec3_add(camera->pos, camera->lookdirection), glms_vec3_cross(camera->right, camera->lookdirection));
    camera->view = glm::lookAt(camera->pos, camera->pos + camera->lookdirection, glm::cross(camera->right, camera->lookdirection));
    // mtx_proj((float *)&camera->proj, camera->fov, camera->ar, camera->near, camera->far, bgfx_get_caps()->homogeneousDepth);
    // camera->proj = glms_perspective(camera->fov, camera->ar, camera->near, camera->far);

    // XXX: TODO: Abstract this based on the standard for renderering backend
    camera->proj = glm::perspective(camera->fov, camera->ar, camera->near, camera->far);
}

void camera_init(struct ORenderer::renderer *renderer) {
    struct camera *camera = (struct camera *)malloc(sizeof(struct camera));
    memset(camera, 0, sizeof(struct camera));
    // renderer->camera = camera;
    // renderer->camera->renderer = renderer;
    // renderer->camera->fov = 60.0f;
    // renderer->camera->ar = 1.0f;
    // renderer->camera->near = 0.1f;
    // renderer->camera->far = 100.0f;
    // renderer->camera->rect = glm::vec2(1.0f, 1.0f);
}
