#ifndef _ENGINE__BOUNDS_HPP
#define _ENGINE__BOUNDS_HPP

#include <engine/math/math.hpp>
#include <engine/matrix.hpp>
#include <stdio.h>
#include <string.h>

// XXX: Notes: using references will reduce memory operations like copies when passing a variable in as an argument

namespace OMath {

    class AABB {
        public:
            glm::vec3 min = glm::vec3(0.0f);
            glm::vec3 max = glm::vec3(0.0f);
           
            AABB(void) { }
            AABB(glm::vec3 min, glm::vec3 max) {
                this->min = min;
                this->max = max;
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
};

// enum {
//     FRUSTUM_PLANENEAR,
//     FRUSTUM_PLANEFAR,
//     FRUSTUM_PLANELEFT,
//     FRUSTUM_PLANERIGHT,
//     FRUSTUM_PLANETOP,
//     FRUSTUM_PLANEBOTTOM,
//     FRUSTUM_PLANEEXTRA0,
//     FRUSTUM_PLANEEXTRA1,
//     FRUSTUM_PLANECOUNT
// };
//
// struct frustum {
//     float xs[FRUSTUM_PLANECOUNT];
//     float ys[FRUSTUM_PLANECOUNT];
//     float zs[FRUSTUM_PLANECOUNT];
//     float ds[FRUSTUM_PLANECOUNT];
//     vec3s points[8];
// };
//
// struct shiftedfrustum {
//     float xs[FRUSTUM_PLANECOUNT];
//     float ys[FRUSTUM_PLANECOUNT];
//     float zs[FRUSTUM_PLANECOUNT];
//     float ds[FRUSTUM_PLANECOUNT];
//     vec3s points[8];
//     vec3s origin;
// } __attribute__((aligned(16)));
//
// static inline struct frustum frustum_init(void) {
//     struct frustum frustum = { 0 };
//     frustum.xs[6] = frustum.xs[7] = 1;
//     frustum.ys[6] = frustum.ys[7] = 0;
//     frustum.zs[6] = frustum.zs[7] = 0;
//     frustum.ds[6] = frustum.ds[7] = 0;
//     return frustum;
// }
//
// static inline bool shiftedfrustum_intersectnearplane(struct shiftedfrustum *frustum, vec3s centre, float radius) {
//     const float x = centre.x - frustum->origin.x;
//     const float y = centre.y - frustum->origin.y;
//     const float z = centre.z - frustum->origin.z;
//     const uint32_t i = FRUSTUM_PLANENEAR;
//     float distance = frustum->xs[i] * x + frustum->ys[i] * y + z * frustum->zs[i] + frustum->ds[i];
//     distance = distance < 0 ? -distance : distance;
//     return distance < radius;
// }
//
// static inline bool frustum_intersectnearplane(struct frustum *frustum, vec3s centre, float radius) {
//     const float x = centre.x;
//     const float y = centre.y;
//     const float z = centre.z;
//     const uint32_t i = FRUSTUM_PLANENEAR;
//     float distance = frustum->xs[i] * x + frustum->ys[i] * y + z * frustum->zs[i] + frustum->ds[i];
//     distance = distance < 0 ? -distance : distance;
//     return distance < radius;
// }
//
// static inline bool frustum_intersectaabboff(struct frustum *frustum, struct aabb aabb, float offset) {
//     vec3s box[] = { aabb.min, aabb.max };
//
//     for (size_t i = 0; i < 6; i++) {
//         int px = frustum->xs[i] > 0.0f;
//         int py = frustum->ys[i] > 0.0f;
//         int pz = frustum->zs[i] > 0.0f;
//
//         float dp = (frustum->xs[i] * box[px].x) + (frustum->ys[i] * box[py].y) + (frustum->zs[i] * box[pz].z);
//
//         if (dp < -frustum->ds[i] - offset) {
//             return false;
//         }
//     }
//     return true;
// }
//
// static inline bool frustum_intersectaabb(struct frustum *frustum, struct aabb aabb) {
//     vec3s box[] = { aabb.min, aabb.max };
//
//     for (size_t i = 0; i < 6; i++) {
//         int px = frustum->xs[i] > 0.0f;
//         int py = frustum->ys[i] > 0.0f;
//         int pz = frustum->zs[i] > 0.0f;
//
//         float dp = (frustum->xs[i] * box[px].x) + (frustum->ys[i] * box[py].y) + (frustum->zs[i] * box[pz].z);
//
//         if (dp < -frustum->ds[i]) {
//             return false;
//         }
//     }
//     return true;
// }
//
// static inline bool shiftedfrustum_intersectaabb(struct shiftedfrustum *frustum, vec3s pos, vec3s size) {
//     const vec3s relpos = glms_vec3_sub(pos, frustum->origin);
//     vec3s box[] = { relpos, glms_vec3_add(relpos, size) };
//
//     for (size_t i = 0; i < 6; i++) {
//         int px = frustum->xs[i] > 0.0f;
//         int py = frustum->ys[i] > 0.0f;
//         int pz = frustum->zs[i] > 0.0f;
//
//         float dp = (frustum->xs[i] * box[px].x) + (frustum->ys[i] * box[py].y) + (frustum->zs[i] * box[pz].z);
//
//         if (dp < -frustum->ds[i]) {
//             return false;
//         }
//     }
//     return true;
// }
//
// static inline void frustum_transform(struct frustum *frustum, mat4s mtx) {
//     for (size_t i = 0; i < 8; i++) {
//         frustum->points[i] = mtx_transformpoint(mtx, frustum->points[i]);
//     }
//
//     for (size_t i = 0; i < sizeof(frustum->xs) / sizeof(float); i++) {
//         vec3s p;
//         if (frustum->xs[i] != 0) {
//             p = (vec3s) { -frustum->ds[i] / frustum->xs[i], 0.0f, 0.0f };
//         } else if (frustum->ys[i] != 0) {
//             p = (vec3s) { 0.0f, -frustum->ds[i] / frustum->ys[i], 0.0f };
//         } else {
//             p = (vec3s) { 0.0f, 0.0f, -frustum->ds[i] / frustum->zs[i] };
//         }
//
//         vec3s n = { frustum->xs[i], frustum->ys[i], frustum->zs[i] };
//         n = mtx_transformvector(mtx, n);
//         p = mtx_transformpoint(mtx, p);
//
//         frustum->xs[i] = n.x;
//         frustum->ys[i] = n.y;
//         frustum->zs[i] = n.z;
//         frustum->ds[i] = -glms_vec3_dot(p, n);
//     }
//
// }
//
// struct sphere frustum_computeboundingsphere(struct frustum *frustum) {
//     struct sphere sphere = { 0 };
//     sphere.pos = frustum->points[0];
//     for (size_t i = 1; i < sizeof(frustum->points) / sizeof(frustum->points[0]); i++) {
//         sphere.pos = glms_vec3_add(sphere.pos, frustum->points[i]);
//     }
//     sphere.pos = glms_vec3_muladds((vec3s) { 1.0f, 1.0f, 1.0f }, 1.0f / (sizeof(frustum->points) / sizeof(frustum->points[0])), sphere.pos);
//
//     sphere.radius = 0;
//     for (size_t i = 0; i < sizeof(frustum->points) / sizeof(frustum->points[0]); i++) {
//         vec3s v = glms_vec3_sub(frustum->points[i], sphere.pos);
//         float lensq = v.x * v.x + v.y * v.y + v.z * v.z;
//         if (lensq > sphere.radius) {
//             sphere.radius = lensq;
//         }
//     }
//     sphere.radius = sqrtf(sphere.radius);
//     return sphere;
// }
//
// static inline bool frustum_issphereinside(struct frustum *frustum, vec3s centre, float radius) {
//     vec4s px = *(vec4s *)(frustum->xs);
//     vec4s py = *(vec4s *)(frustum->ys);
//     vec4s pz = *(vec4s *)(frustum->zs);
//     vec4s pd = *(vec4s *)(frustum->ds);
//
//     vec4s cx = (vec4s) { centre.x, centre.x, centre.x, centre.x };
//     vec4s cy = (vec4s) { centre.y, centre.y, centre.y, centre.y };
//     vec4s cz = (vec4s) { centre.z, centre.z, centre.z, centre.z };
//
//     vec4s t = glms_vec4_mul(cx, px);
//     t = glms_vec4_add(t, glms_vec4_mul(cy, py));
//     t = glms_vec4_add(t, glms_vec4_mul(cz, pz));
//     t = glms_vec4_add(t, pd);
//     t = glms_vec4_sub(t, (vec4s) { -radius, -radius, -radius, -radius });
//
//     if ((t.w < 0 ? (1 << 3) : 0) | (t.z < 0 ? (1 << 2) : 0) | (t.y < 0 ? (1 << 1) : 0) | (t.x < 0 ? 1 : 0)) {
//         return false;
//     }
//
//     px = *(vec4s *)&frustum->xs[4];
//     py = *(vec4s *)&frustum->ys[4];
//     pz = *(vec4s *)&frustum->zs[4];
//     pd = *(vec4s *)&frustum->ds[4];
//
//     t = glms_vec4_mul(cx, px);
//     t = glms_vec4_add(t, glms_vec4_mul(cy, py));
//     t = glms_vec4_add(t, glms_vec4_mul(cz, pz));
//     t = glms_vec4_add(t, pd);
//     t = glms_vec4_sub(t, (vec4s) { -radius, -radius, -radius, -radius });
//
//     return ((t.w < 0 ? (1 << 3) : 0) | (t.z < 0 ? (1 << 2) : 0) | (t.y < 0 ? (1 << 1) : 0) | (t.x < 0 ? 1 : 0)) == 0;
// }
//
// static inline void frustum_setplane(struct frustum *frustum, size_t plane, vec3s normal, vec3s point) {
//     frustum->xs[plane] = normal.x;
//     frustum->ys[plane] = normal.y;
//     frustum->zs[plane] = normal.z;
//     frustum->ds[plane] = -glms_vec3_dot(point, normal);
// }
//
// static inline void shiftedfrustum_setplane(struct shiftedfrustum *frustum, size_t plane, vec3s normal, vec3s point) {
//     frustum->xs[plane] = normal.x;
//     frustum->ys[plane] = normal.y;
//     frustum->zs[plane] = normal.z;
//     frustum->ds[plane] = -glms_vec3_dot(point, normal);
// }
//
// static inline vec3s shiftedfrustum_getnormal(struct shiftedfrustum *frustum, size_t plane) {
//     return (vec3s) { frustum->xs[plane], frustum->ys[plane], frustum->zs[plane] };
// }
//
// static inline struct frustum shiftedfrustum_getrelative(struct shiftedfrustum *frustum, vec3s pos) {
//     struct frustum res = { 0 };
//     const vec3s off = glms_vec3_sub(pos, frustum->origin);
//     memcpy(res.points, frustum->points, sizeof(res.points));
//
//     const vec3s near = shiftedfrustum_getnormal(frustum, FRUSTUM_PLANENEAR);
//     const vec3s far = shiftedfrustum_getnormal(frustum, FRUSTUM_PLANEFAR);
//     const vec3s left = shiftedfrustum_getnormal(frustum, FRUSTUM_PLANELEFT);
//     const vec3s right = shiftedfrustum_getnormal(frustum, FRUSTUM_PLANERIGHT);
//     const vec3s top = shiftedfrustum_getnormal(frustum, FRUSTUM_PLANETOP);
//     const vec3s bottom = shiftedfrustum_getnormal(frustum, FRUSTUM_PLANEBOTTOM);
//
//     frustum_setplane(&res, FRUSTUM_PLANEEXTRA0, near, glms_vec3_add(frustum->points[0], off));
//     frustum_setplane(&res, FRUSTUM_PLANEEXTRA1, near, glms_vec3_add(frustum->points[0], off));
//     frustum_setplane(&res, FRUSTUM_PLANENEAR, near, glms_vec3_add(frustum->points[0], off));
//     frustum_setplane(&res, FRUSTUM_PLANEFAR, far, glms_vec3_add(frustum->points[4], off));
//
//     frustum_setplane(&res, FRUSTUM_PLANELEFT, left, glms_vec3_add(frustum->points[1], off));
//     frustum_setplane(&res, FRUSTUM_PLANERIGHT, right, glms_vec3_add(frustum->points[0], off));
//     frustum_setplane(&res, FRUSTUM_PLANETOP, top, glms_vec3_add(frustum->points[0], off));
//     frustum_setplane(&res, FRUSTUM_PLANEBOTTOM, bottom, glms_vec3_add(frustum->points[2], off));
//
//     for (size_t i = 0; i < 8; i++) {
//         res.points[i] = glms_vec3_add(res.points[i], off);
//     }
//
//     return res;
// }

#endif
