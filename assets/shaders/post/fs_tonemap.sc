$input v_texcoord0

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

SAMPLER2D(s_quadtexture, 0);
SAMPLER2D(s_avglum, 1);

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

vec3 samplebloom(sampler2D tex, vec2 uv) {
    const float threshold = 1.0; // anything above will be bloomed with full contribution (HDR)
    vec3 pixel = texture2DLod(tex, uv, 2.0).rgb;
    if (all(equal(pixel - threshold, vec3_splat(0.0)))) { // conditional expression requires a scalar
        return pixel; // pixel is hdr
    } else {
        return vec3(0.0);
    }
}

void main() {
    vec3 hdr = texture2D(s_quadtexture, v_texcoord0).rgb;
    vec2 ps = 1.0 / textureSize(s_quadtexture, 0);
    vec3 col0 = samplebloom(s_quadtexture, v_texcoord0 + vec2(-ps.x, 0)); // left of pixel 
    vec3 col1 = samplebloom(s_quadtexture, v_texcoord0 + vec2(ps.x, 0)); // right of pixel
    vec3 col2 = samplebloom(s_quadtexture, v_texcoord0 + vec2(0, -ps.y)); // below pixel
    vec3 col3 = samplebloom(s_quadtexture, v_texcoord0 + vec2(0, ps.y)); // above pixel
    vec3 glow = 0.25 * (col0 + col1 + col2 + col3);
    hdr = hdr.rgb + glow.rgb;
    // gl_FragColor = vec4(hdr, 1.0);
    // return;
    float lum = texture2D(s_avglum, v_texcoord0).r;
    // float lum = 0.1;

    vec3 yxy = convertRGB2Yxy(hdr);
    yxy.x /= (9.6 * lum + 0.0001);
    hdr = convertYxy2RGB(yxy);
    // hdr.rgb *= 1.0f;

    // all gamma correction is done here
    vec4 colour = vec4_splat(0.0);
    colour.r = uchimura(hdr.r);
    colour.g = uchimura(hdr.g);
    colour.b = uchimura(hdr.b);
    colour.rgb = toGammaAccurate(colour.rgb);
    colour.a = dot(colour.rgb, vec3(0.299, 0.587, 0.114)); // compute luma (for FXAA)
    gl_FragColor = colour;
    // hdr.r = unreal(hdr.r);
    // hdr.g = unreal(hdr.g);
    // hdr.b = unreal(hdr.b);
    // gl_FragColor = vec4(hdr, 1.0);
    // hdr.r = lottes(hdr.r);
    // hdr.g = lottes(hdr.g);
    // hdr.b = lottes(hdr.b);
    // gl_FragColor = vec4(toGamma(hdr), 1.0);
}
