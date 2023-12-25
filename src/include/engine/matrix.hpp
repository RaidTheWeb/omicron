#ifndef _ENGINE__MATRIX_HPP
#define _ENGINE__MATRIX_HPP

#include <engine/math.hpp>
#include <stdbool.h>
#include <string.h>
//
// static inline vec3s mtx_transformpoint(mat4s mtx, vec3s vec) {
//     return (vec3s) {
//         .x = mtx.col[0].x * vec.x + mtx.col[1].x * vec.y + mtx.col[2].x * vec.z + mtx.col[3].x,
//         .y = mtx.col[0].y * vec.x + mtx.col[1].y * vec.y + mtx.col[2].y * vec.z + mtx.col[3].y,
//         .z = mtx.col[0].z * vec.x + mtx.col[1].z * vec.y + mtx.col[2].z * vec.z + mtx.col[3].z,
//     };
// }
//
// static inline vec3s mtx_transformvector(mat4s mtx, vec3s vec) {
//     return (vec3s) {
//         .x = mtx.col[0].x * vec.x + mtx.col[1].x * vec.y + mtx.col[2].x * vec.z,
//         .y = mtx.col[0].y * vec.x + mtx.col[1].y * vec.y + mtx.col[2].y * vec.z,
//         .z = mtx.col[0].z * vec.x + mtx.col[1].z * vec.y + mtx.col[2].z * vec.z,
//     };
// }
//
// static inline void mtx_lookat(float *res, const vec3s eye, const vec3s at, const vec3s up0) {
//     const vec3s view = glms_normalize(glms_vec3_sub(at, eye));
//     const vec3s uxv = glms_cross(up0, view);
//     const vec3s right = glms_normalize(uxv);
//     const vec3s up1 = glms_cross(view, right);
//
//     memset(res, 0, sizeof(float) * 16);
//     res[0] = right.x;
//     res[1] = up1.x;
//     res[2] = view.x;
//
//     res[4] = right.y;
//     res[5] = up1.y;
//     res[6] = view.y;
//
//     res[8] = right.z;
//     res[9] = up1.z;
//     res[10] = view.z;
//
//     res[12] = -glms_dot(right, eye);
//     res[13] = -glms_dot(up1, eye);
//     res[14] = -glms_dot(view, eye);
//     res[15] = 1.0f;
// }
//
// static inline void mtx_ortho(float *res, float left, float right, float bottom, float top, float near, float far, float offset, bool homogeneousndc) {
//     const float aa = 2.0f / (right - left);
//     const float bb = 2.0f / (top - bottom);
//     const float cc = (homogeneousndc ? 2.0f : 1.0f) / (far - near);
//     const float dd = (left + right) / (left - right);
//     const float ee = (top + bottom) / (bottom - top);
//     const float ff = homogeneousndc ? (near + far) / (near - far) : near / (near - far);
//     memset(res, 0, sizeof(float) * 16);
//     res[0] = aa;
//     res[5] = bb;
//     res[10] = cc;
//     res[12] = dd + offset;
//     res[13] = ee;
//     res[14] = ff;
//     res[15] = 1.0f;
// }
//
// static inline void mtx_projxywh(float *res, float x, float y, float width, float height, float near, float far, bool homogeneousndc) {
//     const float diff = far - near;
//     const float aa = homogeneousndc ? (far + near) / diff : far / diff;
//     const float bb = homogeneousndc ? (2.0f * far * near) / diff : near * aa;
//
//     memset(res, 0, sizeof(float) * 16);
//     res[0] = width;
//     res[5] = height;
//     res[8] = -x;
//     res[9] = -y;
//     res[10] = aa;
//     res[11] = 1.0f;
//     res[14] = -bb;
// }
//
// static inline void mtx_proj(float *res, float fov, float aspect, float near, float far, bool homogeneousndc) {
//     const float height = 1.0f / tanf((fov * 3.1415926535897932384626433832795f / 180.0f) * 0.5f);
//     const float width = height * 1.0f / aspect;
//
//     mtx_projxywh(res, 0.0f, 0.0f, width, height, near, far, homogeneousndc);
// }
//
// static inline void mtx_rotatex(float *res, float ax) {
//     const float sx = sinf(ax);
//     const float cx = cosf(ax);
//
//     memset(res, 0, sizeof(float) * 16);
//     res[0] = 1.0f;
//     res[5] = cx;
//     res[6] = -sx;
//     res[9] = sx;
//     res[10] = cx;
//     res[15] = 1.0f;
// }
//
// static inline void mtx_rotatey(float *res, float ay) {
//     const float sy = sinf(ay);
//     const float cy = cosf(ay);
//
//     memset(res, 0, sizeof(float) * 16);
//     res[0] = cy;
//     res[2] = sy;
//     res[5] = 1.0f;
//     res[8] = -sy;
//     res[10] = cy;
//     res[15] = 1.0f;
// }
//
// static inline void mtx_rotatez(float *res, float az) {
//     const float sz = sinf(az);
//     const float cz = cosf(az);
//
//     memset(res, 0, sizeof(float) * 16);
//     res[0] = cz;
//     res[1] = -sz;
//     res[4] = sz;
//     res[5] = cz;
//     res[10] = 1.0f;
//     res[15] = 1.0f;
// }
//
// static inline void mtx_rotatexy(float *res, float ax, float ay) {
//     const float sx = sinf(ax);
//     const float cx = cosf(ax);
//     const float sy = sinf(ay);
//     const float cy = cosf(ay);
//
//     memset(res, 0, sizeof(float) * 16);
//     res[0] = cy;
//     res[2] = sy;
//     res[4] = sx * sy;
//     res[5] = cx;
//     res[6] = -sx * cy;
//     res[8] = -cx * sy;
//     res[9] = sx;
//     res[10] = cx * cy;
//     res[15] = 1.0f;
// }
//
// static inline void mtx_rotatexyz(float *res, float ax, float ay, float az) {
//     const float sx = sinf(ax);
//     const float cx = cosf(ax);
//     const float sy = sinf(ay);
//     const float cy = cosf(ay);
//     const float sz = sinf(az);
//     const float cz = cosf(az);
//
//     memset(res, 0, sizeof(float) * 16);
//     res[0] = cy * sz;
//     res[1] = -cy * sz;
//     res[2] = sy;
//     res[4] = cz * sx * sy + cx * sz;
//     res[5] = cx * cz - sx * sy * sz;
//     res[6] = -cy * sx;
//     res[8] = -cx * cz * sy + sx * sz;
//     res[9] = cz * sx + cx * sy * sz;
//     res[10] = cx * cy;
//     res[15] = 1.0f;
// }
//
// static inline void mtx_rotatezyx(float *res, float ax, float ay, float az) {
//     const float sx = sinf(ax);
//     const float cx = cosf(ax);
//     const float sy = sinf(ay);
//     const float cy = cosf(ay);
//     const float sz = sinf(az);
//     const float cz = cosf(az);
//
//     memset(res, 0, sizeof(float) * 16);
//     res[0] = cy * sz;
//     res[1] = cz * sx * sy - cx * sz;
//     res[2] = cx * cz * sy + sx * sz;
//     res[4] = cz * sz;
//     res[5] = cx * cz + sx * sy * sz;
//     res[6] = -cy * sx + cx * sy * sz;
//     res[8] = -cx * cz * sy + sx * sz;
//     res[9] = -sy;
//     res[10] = cx * cy;
//     res[15] = 1.0f;
// }
//
// static inline void mtx_scale(float *res, float ax, float ay, float az) {
//     memset(res, 0, sizeof(float) * 16);
//
//     res[0] = ax;
//     res[5] = ay;
//     res[10] = az;
//     res[15] = 1.0f;
// }

#endif
