#include <bgfx_compute.sh>
#include "../shaderlib.sh"

#define GROUP_SIZE 256
#define THREADS_X 16
#define THREADS_Y 16

#define EPSILON 0.000001
#define RGB_LUM vec3(0.2125, 0.7154, 0.0721)

uniform vec4 u_histparams;
#define u_params0 u_histparams

IMAGE2D_RO(s_screentexture, rgba16f, 0);
BUFFER_RW(histogram, uint, 1);

SHARED uint histogramShared[GROUP_SIZE];

uint colortobin(vec3 hdr, float minlum, float inverselumrange) {
    float lum = dot(hdr, RGB_LUM);

    if (lum < EPSILON) {
        return 0;
    }

    float loglum = clamp((log2(lum) - minlum) * inverselumrange, 0.0, 1.0);

    return uint(loglum * 254.0 + 1.0);
}

uint lumtobin(float lum, float minlum, float inverselumrange) {
    if (lum < 1e-3) {
        return 0;
    }

    float loglum = clamp((log2(lum) - minlum) * inverselumrange, 0.0, 1.0);

    return uint(loglum * 254.0 + 1.0);
}

NUM_THREADS(THREADS_X, THREADS_Y, 1)
void main() {
    histogramShared[gl_LocalInvocationIndex] = 0;
    histogram[gl_LocalInvocationIndex] = 0;
    groupMemoryBarrier();
    barrier();

    uvec2 dim = uvec2(u_viewRect.zw);

    if (gl_GlobalInvocationID.x < dim.x && gl_GlobalInvocationID.y < dim.y) {
        vec3 hdr = imageLoad(s_screentexture, ivec2(gl_GlobalInvocationID.xy)).rgb;
        uint binidx = colortobin(hdr, u_params0.x, u_params0.y);
        atomicAdd(histogramShared[binidx], 1); // add respecting multi-threaded
    }

    // if (all(lessThan(gl_LocalInvocationID.xy, uvec2(u_viewRect.zw + 0.5)))) {
    //     float lum = dot(RGB_LUM, imageLoad(s_screentexture, ivec2(ivec2(gl_GlobalInvocationID.xy) / u_viewRect.zw)).rgb);
    //     uint bin = lumtobin(lum, u_params0.x, 1 / u_params0.y);
    //     // uint bin = lumtobin(lum, -6, 5);
    //     atomicAdd(histogramShared[bin], 1);
    // }

    groupMemoryBarrier(); // wait until all threads hit this point in our shader
    barrier();

    atomicAdd(histogram[gl_LocalInvocationIndex], histogramShared[gl_LocalInvocationIndex]); // add this shared buffer into the output buffer
}

/* vim: set sw=4 ts=4 expandtab: */
