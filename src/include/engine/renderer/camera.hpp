#ifndef _ENGINE__CAMERA_HPP
#define _ENGINE__CAMERA_HPP

#include <engine/math/bounds.hpp>
#include <engine/math/math.hpp>
#include <engine/renderer/renderer.hpp>

namespace ORenderer {
    class PerspectiveCamera {
        public:
            // Horizontal Field of View
            float fov = 60.0f;
            // Aspect Ratio (w / h)
            float ar = 16.0f / 9.0f;

            // Clipping planes (near)
            float near = 0.1f;
            // Clipping planes (far)
            float far = 100.0f;

            // World space position
            glm::vec3 pos;
            // World space orientation
            glm::quat orientation = glm::quat(glm::vec3(0.0f));

            enum flags {
                VIEW = (1 << 0),
                PROJ = (1 << 1),
                ALL = VIEW | PROJ
            };

            // Projection (perspective)
            glm::mat4 proj = glm::mat4(1.0f);
            // World to view transform
            glm::mat4 view = glm::mat4(1.0f);
            glm::mat4 viewproj = glm::mat4(1.0f); // proj * view
            glm::mat4 invviewproj = glm::mat4(1.0f); // inverse(proj * view)

            // Frustum
            OMath::Frustum frustum;
            std::atomic<uint8_t> dirty;

            PerspectiveCamera(const glm::vec3 &pos, const glm::quat &orientation, float fov, float ar, float near = 0.1f, float far = 100.0f) {
                this->dirty.store(flags::ALL);
                this->pos = pos;
                this->orientation = orientation;
                this->fov = fov;
                this->ar = ar;
                this->near = near;
                this->far = far;
            }

            PerspectiveCamera(PerspectiveCamera &cam) {
                this->fov = cam.fov;
                this->ar = cam.ar;
                this->orientation = cam.orientation;
                this->near = cam.near;
                this->far = cam.far;
                this->proj = cam.proj;
                this->view = cam.view;
                this->viewproj = cam.viewproj;
                this->invviewproj = cam.invviewproj;
                this->frustum = cam.frustum;
                this->dirty.store(cam.dirty.load());
                this->pos = cam.pos;
            }

            void setfov(float fov) {
                if (this->fov != fov) {
                    this->fov = fov;
                    this->dirty.store(this->dirty.load() | flags::PROJ);
                }
            }

            void setpos(const glm::vec3 &pos) {
                if (this->pos != pos) {
                    this->pos = pos;
                    this->dirty.store(this->dirty.load() | flags::VIEW);
                }
            }

            void setorientation(const glm::quat &orientation) {
                if (this->orientation != orientation) {
                    this->orientation = orientation;
                    this->dirty.store(this->dirty.load() | flags::VIEW);
                }
            }

            void update(void) {
                if (!this->dirty.load()) {
                    return;
                }

                uint8_t flags = this->dirty.load();
                if (flags & flags::VIEW) {
                    glm::mat4 t = glm::translate(glm::mat4(1.0f), -this->pos);
                    glm::mat4 r = glm::mat4_cast(this->orientation);
                    this->view = r * t;
                    // this->view = glm::lookAt(this->pos, this->pos + this->getforward(), glm::vec3(0.0f, 1.0f, 0.0f));
                    // this->view = glm::lookAt(this->pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    this->dirty.store(this->dirty.load() & ~flags::VIEW);
                }

                if (flags & flags::PROJ) {
                    this->proj = glm::perspective(glm::radians(this->fov), this->ar, this->near, this->far);
                    context->adjustprojection(&this->proj);
                    this->dirty.store(this->dirty.load() & ~flags::PROJ);
                }

                if ((flags & flags::VIEW) || (flags & flags::PROJ)) {
                    this->viewproj = proj * view;
                    this->invviewproj = glm::inverse(this->viewproj);
                }

                if (flags != 0) {
                    // glm::mat4 viewproj = this->viewproj;
                    // context->adjustprojection(&viewproj);
                    this->frustum.update(viewproj);
                    // this->frustum.update(this->pos, this->getforward(), this->getup(), this->getright(), this->near, this->far, this->fov, this->ar);
                }
            }

            glm::vec3 getright(void) {
                return this->orientation * OMath::worldright;
            }

            glm::vec3 getup(void) {
                return this->orientation * OMath::worldup;
            }

            glm::vec3 getforward(void) {
                return this->orientation * OMath::worldfront;
            }

            glm::mat4 getview(void) {
                this->update();
                return this->view;
            }

            glm::mat4 getproj(void) {
                this->update();
                return this->proj;
            }

            glm::mat4 getviewproj(void) {
                this->update();
                return this->viewproj;
            }

            glm::mat4 getinvviewproj(void) {
                this->update();
                return this->invviewproj;
            }

            OMath::Frustum &getfrustum(void) {
                this->update();
                return this->frustum;
            }

    };
}


#endif
