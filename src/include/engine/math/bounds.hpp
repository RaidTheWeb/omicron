#ifndef _ENGINE__BOUNDS_HPP
#define _ENGINE__BOUNDS_HPP

#include <engine/math/math.hpp>
#include <engine/matrix.hpp>
#include <tracy/Tracy.hpp>
#include <stdio.h>
#include <string.h>

// XXX: Notes: using references will reduce memory operations like copies when passing a variable in as an argument

namespace OMath {

    class AABB {
        public:
            glm::vec3 min = glm::vec3(0.0f);
            glm::vec3 max = glm::vec3(0.0f);
            glm::vec3 centre = glm::vec3(0.0f);
            glm::vec3 extent = glm::vec3(0.0f);

            AABB(void) { }
            AABB(glm::vec3 min, glm::vec3 max) {
                this->min = min;
                this->max = max;
                this->centre = (this->max + this->min) * 0.5f;
                this->extent = this->max - this->min;
            }

            glm::vec3 mincoords(glm::vec3 a, glm::vec3 b) {
                return glm::vec3(glm::min(a.x, b.x), glm::min(a.y, b.y), glm::min(a.z, b.z));
            }

            glm::vec3 maxcoords(glm::vec3 a, glm::vec3 b) {
                return glm::vec3(glm::max(a.x, b.x), glm::max(a.y, b.y), glm::max(a.z, b.z));
            }

            // Modify AABB to emcompass a point.
            void addpt(glm::vec3 point) {
                this->min = this->mincoords(point, this->min);
                this->max = this->maxcoords(point, this->max);
            }

            // Merge this AABB with another (modify bounds to emcompass the points of the AABB).
            void merge(AABB rhs) {
                this->addpt(rhs.min);
                this->addpt(rhs.max);
            }

            // Check if AABB contains a point.
            bool contains(glm::vec3 point) {
                if (this->min.x > point.x || this->min.y > point.y || this->min.z > point.z) {
                    return false;
                }
                if (this->max.x < point.x || this->max.y < point.y || this->max.z < point.z) {
                    return false;
                }
                return true;
            }

            // Check if AABB intersects with another AABB.
            bool intersects(AABB rhs) {
                if (this->min.x > rhs.max.x || this->min.y > rhs.max.y || this->min.z > rhs.max.z) {
                    return false;
                }
                if (this->max.x < rhs.min.x || this->max.y < rhs.min.y || this->max.z < rhs.min.z) {
                    return false;
                }
                return true;
            }

            // Get the centre point of the AABB.
            glm::vec3 getcentre(void) {
                return (this->max + this->min) * 0.5f;
            }

            glm::vec3 calcextent(void) {
                return this->max - this->min;
            }

            // Get the radius of the AABB.
            float radius(void) {
                return glm::length(this->calcextent() * 0.5f);
            }

            void transform(glm::mat4 mtx) {
                const glm::vec4 xa = mtx[0] * this->min.x;
                const glm::vec4 xb = mtx[0] * this->max.x;
                const glm::vec4 ya = mtx[1] * this->min.y;
                const glm::vec4 yb = mtx[1] * this->max.y;
                const glm::vec4 za = mtx[2] * this->min.z;
                const glm::vec4 zb = mtx[2] * this->max.z;

                // Transform min and max
                this->min = glm::min(xa, xb) + glm::min(ya, yb) + glm::min(za, zb) + mtx[3];
                this->max = glm::max(xa, xb) + glm::max(ya, yb) + glm::max(za, zb) + mtx[3];
            }

            AABB transformed(glm::mat4 mtx) {
                const glm::vec4 xa = mtx[0] * this->min.x;
                const glm::vec4 xb = mtx[0] * this->max.x;
                const glm::vec4 ya = mtx[1] * this->min.y;
                const glm::vec4 yb = mtx[1] * this->max.y;
                const glm::vec4 za = mtx[2] * this->min.z;
                const glm::vec4 zb = mtx[2] * this->max.z;

                // Transform min and max
                return AABB(glm::min(xa, xb) + glm::min(ya, yb) + glm::min(za, zb) + mtx[3], max = glm::max(xa, xb) + glm::max(ya, yb) + glm::max(za, zb) + mtx[3]);
            }

            constexpr bool operator ==(AABB &rhs) {
                return this->min == rhs.min && this->max == rhs.max;
            }

            constexpr bool operator !=(AABB &rhs) {
                return this->min != rhs.min || this->max != rhs.max;
            }

            // Create AABB that represents the intersecting area between two AABBs.
            AABB intersection(AABB rhs) {
                return AABB(glm::max(rhs.min, this->min), glm::min(rhs.max, this->max));
            }

            // Translate AABB position.
            void translate(glm::vec3 v) {
                this->min += v;
                this->max += v;
            }
    };

    class OBB {
        public:
            // glm::vec3 min = glm::vec3(0.0f);
            // glm::vec3 max = glm::vec3(0.0f);
            AABB bounds;
            glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            OBB(void) { }
            OBB(AABB aabb, glm::quat orientation) {
                this->bounds = aabb;
                // this->min = aabb.min;
                // this->max = aabb.max;
                this->orientation = orientation;
            }

            // AABB toaabb(void) {
                // return OMath::AABB(this->min, this->max);
            // }
    };

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

    class Plane {
        public:
            glm::vec3 normal = glm::vec3(0.0f);
            float distance = 0.0f;

            Plane(void) { }

            Plane(const glm::vec3 &p1, const glm::vec3 &norm) {
                this->normal = glm::normalize(norm);
                this->distance = glm::dot(this->normal, p1);
            }

            Plane(const glm::vec3 &normal, float distance) {
                this->normal = normal;
                this->distance = distance;
            }
    };

    class Frustum {
        public:
            enum planes {
                NEAR,
                FAR,
                LEFT,
                RIGHT,
                TOP,
                BOTTOM,
                COUNT
            };

            enum intersection {
                OUTSIDE,
                INTERSECT,
                INSIDE
            };

            Plane planes[COUNT];

            Frustum(void) { }

            void update(glm::mat4 &mtx) {
                ZoneScopedN("Update Frustum");

                // This big chunk of code extracts individual planes from the matrix. This matrix can be view, proj, or viewproj depending on what this frustum is used for.
                this->planes[planes::LEFT] = Plane(
                    glm::vec3(mtx[0][3] + mtx[0][0], mtx[1][3] + mtx[1][0], mtx[2][3] + mtx[2][0]),
                    mtx[3][3] + mtx[3][0]
                );
                this->planes[planes::RIGHT] = Plane(
                    glm::vec3(mtx[0][3] - mtx[0][0], mtx[1][3] - mtx[1][0],mtx[2][3] - mtx[2][0]),
                    mtx[3][3] - mtx[3][0]
                );
                this->planes[planes::BOTTOM] = Plane(
                    glm::vec3(mtx[0][3] + mtx[0][1], mtx[1][3] + mtx[1][1], mtx[2][3] + mtx[2][1]),
                    mtx[3][3] + mtx[3][1]
                );
                this->planes[planes::TOP] = Plane(
                    glm::vec3(mtx[0][3] - mtx[0][1], mtx[1][3] - mtx[1][1], mtx[2][3] - mtx[2][1]),
                    mtx[3][3] - mtx[3][1]
                );
                this->planes[planes::NEAR] = Plane(
                    glm::vec3(mtx[0][3] + mtx[0][2], mtx[1][3] + mtx[1][2], mtx[2][3] + mtx[2][2]),
                    mtx[3][3] + mtx[3][2]
                );
                this->planes[planes::FAR] = Plane(
                    glm::vec3(mtx[0][3] - mtx[0][2], mtx[1][3] - mtx[1][2], mtx[2][3] - mtx[2][2]),
                    mtx[3][3] - mtx[3][2]
                );

                // Normalise all the planes.
                for (size_t i = 0; i < planes::COUNT; i++) {
                    const float magnitude = glm::length(this->planes[i].normal);
                    this->planes[i].normal /= magnitude;
                    this->planes[i].distance /= magnitude;
                }
            }

            Frustum(glm::mat4 mtx) {
                this->update(mtx);
            }

            bool intersectnearplane(Sphere sphere) {
                const Plane plane = this->planes[planes::NEAR];
                const float distance = glm::dot(plane.normal, sphere.pos) + plane.distance;
                return distance < -sphere.radius ? false : true;
            }

            bool intersectpoint(glm::vec3 point) {
                for (size_t i = 0; i < planes::COUNT; i++) {
                    const Plane &plane = this->planes[i];
                    if (glm::dot(plane.normal, point) + plane.distance < 0) {
                        return false;
                    }
                }
                return true;
            }

            size_t testsphere(Sphere sphere) {
                ZoneScopedN("Sphere Test");
                size_t result = intersection::INSIDE;
                for (size_t i = 0; i < planes::COUNT; i++) {
                    const Plane &plane = this->planes[i];
                    const float c = glm::dot(plane.normal, sphere.pos) + plane.distance;
                    if (c < -sphere.radius) {
                        return intersection::OUTSIDE;
                    }
                    if (glm::abs(c) < sphere.radius) {
                        result = intersection::INTERSECT;
                    }
                }
                return result;
            }

            size_t testobb(OBB obb) {
                ZoneScopedN("OBB Test");
                // XXX: Unreliable
                size_t result = intersection::INSIDE;
                const glm::vec3 centre = obb.bounds.centre;
                const glm::vec3 size = obb.bounds.extent;
                for (size_t i = 0; i < planes::COUNT; i++) {
                    const Plane &plane = this->planes[i];

                    const glm::vec3 normal = glm::rotate(obb.orientation, plane.normal);
                    const float r = fabs(size.x * normal.x) + fabs(size.y * normal.y) + fabs(size.z * normal.z);
                    const float d = glm::dot(plane.normal, centre) + plane.distance;

                    const float ret = fabs(d) < r ? 0.0f : d < 0.0f ? d + r : d - r;
                    if (ret == 0.0f) {
                        result = intersection::INTERSECT;
                    } else if (ret < 0.0f) {
                        return intersection::OUTSIDE;
                    }
                }
                return result;
            }

            size_t testaabb(AABB aabb) {
                size_t result = intersection::INSIDE;
                for (size_t i = 0; i < planes::COUNT; i++) {
                    Plane plane = this->planes[i];

                    AABB representation = aabb;

                    if (plane.normal.x >= 0) {
                        representation.min.x = aabb.max.x;
                        representation.max.x = aabb.min.x;
                    }
                    if (plane.normal.y >= 0) {
                        representation.min.y = aabb.max.y;
                        representation.max.y = aabb.min.y;
                    }
                    if (plane.normal.z >= 0) {
                        representation.min.z = aabb.max.z;
                        representation.max.z = aabb.min.z;
                    }
                    if ((glm::dot(plane.normal, representation.min) + plane.distance) < 0) {
                        return intersection::OUTSIDE;
                    }
                    if ((glm::dot(plane.normal, representation.max) + plane.distance) < 0) {
                        result = intersection::INTERSECT;
                    }
                }
                return result;
            }
    };
};

#endif
