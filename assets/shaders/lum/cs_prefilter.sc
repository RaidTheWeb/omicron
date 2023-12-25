#include <bgfx_compute.sh>
#include "../shaderlib.sh"

// TODO: This should probably be resized first to a lower resolution to save on bandwidth (highly unlikely we'll ever have a smooth mirror surface that is intended to reflect everything accurately)

#define PI 3.141592653589793

#define THREADS 8
#define SAMPLES 64

uniform vec4 u_params[1];
#define u_params0 u_params[0]

SAMPLERCUBE(s_prefiltercubemap, 0);
IMAGE2D_ARRAY_WR(s_target, rgba16f, 1);

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

float distributionggx(vec3 N, vec3 H, float rough) {
    float a = rough * rough;
    float a2 = a * a;

    float nDotH = max(dot(N, H), 0.0);
    float nDotH2 = nDotH * nDotH;

    float num = a2;
    float denom = (nDotH2 * (a2 - 1.0) + 1.0);
    denom = 1 / (PI * denom * denom);

    return num * denom;
}

vec3 prefilter(float roughness, vec3 R, float imgsize) {
    vec3 N = R;
    vec3 V = R;
    vec3 prefiltered = vec3_splat(0.0);
    float total = 0.0;

    for (uint i = 0; i < SAMPLES; i++) {
        vec2 Xi = hammersley(i, SAMPLES);
        vec3 H = importanceggx(Xi, roughness, N);
        float VoH = dot(V, H);
        float NoH = VoH;
        vec3 L = 2.0 * VoH * H - V;
        float NoL = saturate(dot(N, L));
        NoH = saturate(NoH);

        if (NoL > 0.0) {
            float pdf = distributionggx(V, H, roughness) / 4.0 + 0.001;
            float omegas = 1.0 / (float(SAMPLES) * pdf);
            float omegap = 4.0 * PI / (6.0 * imgsize * imgsize);
            float mip = roughness == 0.0 ? 0.0 : max(0.5 * log2(omegas / omegap), 0.0);
            prefiltered += textureCubeLod(s_prefiltercubemap, L, mip).rgb * NoL;
            total += NoL;
        }
    }
    return prefiltered / total;
}

NUM_THREADS(THREADS, THREADS, 6)
void main() {
    float mip = u_params0.y;
    float imgsize = u_params0.z;
    float mipimgsize = imgsize / pow(2.0, mip);
    ivec3 global = ivec3(gl_GlobalInvocationID.xyz);

    if (global.x >= mipimgsize || global.y >= mipimgsize) {
        return;
    }

    vec3 R = normalize(toWorldCoords(global, mipimgsize));
    if (u_params0.x == 0.0) {
        vec4 colour = textureCubeLod(s_prefiltercubemap, R, 0);
        imageStore(s_target, global, colour);
        return;
    }

    vec3 colour = prefilter(u_params0.x, R, imgsize);
    imageStore(s_target, global, vec4(colour, 1.0));
}

 /* vim: set sw=4 ts=4 expandtab: */
