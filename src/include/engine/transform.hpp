#ifndef _ENGINE__TRANSFORM_HPP
#define _ENGINE__TRANSFORM_HPP

#include <engine/math.hpp>

#define TRANSFORM_QUATERNIONIDENTITY glm::quat(1.0f, 0.0f, 0.0f, 0.0f)

class Transform {
    private:
        glm::mat4 mtx;
    public:
        // these variables will all be local transforms
        glm::vec3 position;
        glm::quat orientation;
        glm::vec3 scale;
        Transform *parent;

    Transform(glm::vec3 position = glm::vec3(0.0f), glm::quat orientation = TRANSFORM_QUATERNIONIDENTITY, glm::vec3 scale = glm::vec3(1.0f), Transform *inherit = NULL) {
        this->position = position;
        this->orientation = orientation;
        this->scale = scale;
        this->parent = inherit;

        this->mtx = this->getlocalmatrix();
    }
    Transform(glm::mat4 m) {
        glm::vec3 skew;
        glm::vec4 perspective;
        (void)skew;
        (void)perspective;

        glm::decompose(m, this->scale, this->orientation, this->position, skew, perspective);
        this->mtx = m;
    }

    glm::mat4 getlocalmatrix(void) {
        return glm::translate(position) * glm::toMat4(orientation) * glm::scale(scale);
    }

    glm::mat4 getmatrix(void) {
        return this->parent != NULL ? this->parent->getmatrix() * this->getlocalmatrix() : this->getlocalmatrix(); // resolve absolute matrix
    }

    glm::vec3 getposition(void) {
        return this->parent != NULL ? this->parent->getposition() + this->position : this->position; // resolve absolute position
    }

    glm::quat getorientation(void) {
        return this->parent != NULL ? glm::normalize(this->parent->getorientation() * this->orientation) : this->orientation; // resolve absolute orientation
    }

    glm::vec3 getscale(void) {
        return this->parent != NULL ? this->parent->getscale() * this->scale : this->scale; // resolve absolute scale
    }

    Transform &translate(glm::vec3 v) {
        this->position += v;
        return *this;
    }

    Transform &rotate(glm::quat q) {
        this->orientation = glm::normalize(this->orientation * q);
        return *this;
    }

    Transform &pitch(float angle) {
        return this->rotate(glm::quat(glm::vec3(angle, 0.0f, 0.0f)));
    }

    Transform &yaw(float angle) {
        return this->rotate(glm::quat(glm::vec3(0.0f, angle, 0.0f)));
    }

    Transform &roll(float angle) {
        return this->rotate(glm::quat(glm::vec3(0.0f, 0.0f, angle)));
    }

    Transform &lookat(glm::vec4 target) {
        this->orientation = glm::quatLookAt(-glm::normalize(target.w ? glm::vec3(target) - this->position : glm::vec3(target)), glm::vec3(0.0f, 1.0f, 0.0f));
        return *this;
    }

    Transform &scalev(glm::vec3 s) {
        this->scale *= s;
        return *this;
    }
};

#endif
