
#include <bgfx_compute.sh>

#define PI 3.141592653589793

#define GROUP_SIZE 256
#define THREADS 16

IMAGE2D_WR(s_target, rg16f, 0);

vec2 hammersley(uint i, uint N) {
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10;
    return vec2(float(i) /float(N), rdi);
}

vec3 importanceggx(vec2 Xi, float roughness, vec3 N) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    vec3 H = vec3(
        sinTheta * cos(phi),
        sinTheta * sin(phi),
        cosTheta
    );

    vec3 upVector = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangentX = normalize(cross(upVector, N));
    vec3 tangentY = normalize(cross(N, tangentX));
    return normalize(tangentX * H.x + tangentY * H.y + N * H.z);
}

float smithggx(float NoV, float NoL, float roughness) {
    float a2 = pow(roughness, 4.0);
    float ggxv = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float ggxl = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (ggxv + ggxl);
}

vec2 integratebrdf(float roughness, float NoV) {
	vec3 V;
    V.x = sqrt(1.0 - NoV * NoV); // sin
    V.y = 0.0;
    V.z = NoV; // cos

    const vec3 N = vec3(0.0, 0.0, 1.0);

    float A = 0.0;
    float B = 0.0;
    const uint numsamples = 1024;

    for (uint i = 0u; i < numsamples; i++) {
        vec2 Xi = hammersley(i, numsamples);
        vec3 H = importanceggx(Xi, roughness, N);

        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NoL = saturate(dot(N, L));
        float NoH = saturate(dot(N, H));
        float VoH = saturate(dot(V, H));

        if (NoL > 0.0) {
            float pdf = smithggx(NoV, NoL, roughness) * VoH * NoL / NoH;
            float Fc = pow(1.0 - VoH, 5.0);
            A += (1.0 - Fc) * pdf;
            B += Fc * pdf;
        }
    }

    return 4.0 * vec2(A, B) / float(numsamples);
}


NUM_THREADS(THREADS, THREADS, 1)
void main() {
    vec2 uv = vec2(gl_GlobalInvocationID.xy) / vec2(imageSize(s_target).xy);
    float mu = uv.x;
    float a = max(uv.y, 0.045);

    vec2 res = integratebrdf(a, mu);
    imageStore(s_target, ivec2(gl_GlobalInvocationID.xy), vec4(res, 0.0, 0.0));
}

/* vim: set sw=4 ts=4 expandtab: */
