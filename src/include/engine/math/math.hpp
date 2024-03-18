#ifndef _ENGINE__MATH__MATH_HPP
#define _ENGINE__MATH__MATH_HPP

// TODO: Just transform to left handed whenever we want to (although, BGFX does seem to like left handedness)
// #define CGLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#ifdef OMICRON_RENDERVULKAN // Vulkan rendering
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <math.h>

namespace OMath {

    class Sphere {
        public:
            Sphere(void) { }
            Sphere(glm::vec3 pos, float radius) {
                this->pos = pos;
                this->radius = radius;
            }

            glm::vec3 pos;
            float radius;
    };
}

#endif
