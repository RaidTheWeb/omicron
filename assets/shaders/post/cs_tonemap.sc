#include <bgfx_compute.sh>
#include "../shaderlib.sh"

// Tonemapping compute shader

SAMPLER2D(s_quadtexture, 0);
SAMPLER2D(s_avglum, 1);
IMAGE2D_WR(s_target, rgba8, 2);

float aces(float x) {
    return (x * (2.51 * x + 0.03)) / (x * (2.43 * x * 0.59) + 0.14);
}

float unreal(float x) {
    // Unreal Engine 3
    // Does not collaborate well with bgfx's tendency to utilise sRGB backbuffers
    // for seemingly no reason (you need to convert the input framebuffer texture to linear first so it doesn't double gamma correct)
    // This one appears to give the result a reddish tint
    return x / (x + 0.155) * 1.019;
}

float uchimura(float x, float maxbright, float contrast, float linearstart, float linearlength, float black, float pedestal) {
    // Uchimura "HDR theory and practise"
    // Gran Turismo
    // This one looks a lot closer to how it appears in blender
    float l0 = ((maxbright - linearstart) * linearlength) / contrast;
    float L0 = linearstart - linearstart / contrast;
    float L1 = linearstart + (1.0 - linearstart) / contrast;
    float S0 = linearstart + l0;
    float S1 = linearstart + contrast * l0;
    float C2 = (contrast * maxbright) / (maxbright - S1);
    float CP = -C2 / maxbright;

    float w0 = 1.0 - smoothstep(0.0, linearstart, x);
    float w2 = step(linearstart + l0, x);
    float w1 = 1.0 - w0 - w2;

    float T = linearstart * pow(x / linearstart, black) + pedestal;
    float S = maxbright - (maxbright - S1) * exp(CP * (x - S0));
    float L = linearstart + contrast * (x - linearstart);

    return T * w0 + L * w1 + S * w2;
}

float uchimura(float x) {
    const float maxbright = 1.0;
    const float contrast = 1.0;
    const float linearstart = 0.22;
    const float linearlength = 0.4;
    const float black = 1.33;
    const float pedestal = 0.0;

    return uchimura(x, maxbright, contrast, linearstart, linearlength, black, pedestal);
}

float lottes(float x) {
    // Lottes "Advanced Techniques and Optimization of HDR Color Pipelines"
    // Closer to texture
    const float a = 1.6;
    const float d = 0.977;
    const float hdrmax = 8.0;
    const float midin = 0.18;
    const float midout = 0.267;

    const float b = (-pow(midin, a) + pow(hdrmax, a) * midout) / ((pow(hdrmax, a * d) - pow(midin, a * d)) * midout);
    const float c = (pow(hdrmax, a * d) * pow(midin, a) - pow(hdrmax, a) * pow(midin, a * d) * midout) / ((pow(hdrmax, a * d) - pow(midin, a * d)) * midout);
    return pow(x, a) / (pow(x, a * d) * b + c);
}

NUM_THREADS(16, 16, 1)
void main() {
    vec2 uv = vec2(gl_GlobalInvocationID.xy) / (imageSize(s_target).xy);
    vec3 hdr = texture2D(s_quadtexture, uv).rgb;
    float lum = texture2D(s_avglum, vec2(0, 0)).r;

    vec3 yxy = convertRGB2Yxy(hdr);
    yxy.x /= (9.6 * lum + 0.0001);
    hdr = convertYxy2RGB(yxy);

    // all gamma correction is done here
    vec4 colour = vec4_splat(0.0);
    colour.r = lottes(hdr.r);
    colour.g = lottes(hdr.g);
    colour.b = lottes(hdr.b);
    colour.rgb = toGamma(colour.rgb);
    colour.a = sqrt(dot(colour.rgb, vec3(0.299, 0.587, 0.114))); // compute luma (for FXAA)
    imageStore(s_target, ivec2(gl_GlobalInvocationID.xy), colour);
    // hdr.r = unreal(hdr.r);
    // hdr.g = unreal(hdr.g);
    // hdr.b = unreal(hdr.b);
    // gl_FragColor = vec4(hdr, 1.0);
    // hdr.r = lottes(hdr.r);
    // hdr.g = lottes(hdr.g);
    // hdr.b = lottes(hdr.b);
    // gl_FragColor = vec4(toGamma(hdr), 1.0);

}

/* vim: set sw=4 ts=4 expandtab: */
