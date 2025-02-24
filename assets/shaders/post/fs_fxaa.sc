$input v_texcoord0

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

SAMPLER2D(s_quadtexture, 0);

// FXAA 3.11 by Timothy Lottes
//-----

#define FXAA_PC 1
#define FXAA_GLSL_120 1
#define FXAA_GREEN_AS_LUMA 1
#define FXAA_QUALITY__PRESET 39
#define FXAA_FAST_PIXEL_OFFSET 0
#define FXAA_GATHER4_ALPHA 0
#include "fxaa3_11.h"

uniform vec4 u_params[1];

vec4 fxaa_311(sampler2D _sampler, vec2 _coord, vec2 _invScrSize, float _quality, float _treshold, float _tresholdMin) {
    return FxaaPixelShader(_coord*_invScrSize //pos
                         , vec4_splat(0.0)    //fxaaConsolePosPos              - placeholder
                         , _sampler           //tex
                         , _sampler           //fxaaConsole360TexExpBiasNegOne - placeholder
                         , _sampler           //fxaaConsole360TexExpBiasNegTwo - placeholder
                         , _invScrSize        //fxaaQualityRcpFrame
                         , vec4_splat(0.0)    //fxaaConsoleRcpFrameOpt         - placeholder
                         , vec4_splat(0.0)    //fxaaConsoleRcpFrameOpt2        - placeholder
                         , vec4_splat(0.0)    //fxaaConsole360RcpFrameOpt2     - placeholder
                         , _quality           //fxaaQualitySubpix
                         , _treshold          //fxaaQualityEdgeThreshold
                         , _tresholdMin       //fxaaQualityEdgeThresholdMin
                         , 0.0                //fxaaConsoleEdgeSharpness       - placeholder
                         , 0.0                //fxaaConsoleEdgeThreshold       - placeholder
                         , 0.0                //fxaaConsoleEdgeThresholdMin    - placeholder
                         , vec4_splat(0.0)    //fxaaConsole360ConstDir         - placeholder
                         );
}

vec4 fxaa_311(sampler2D _sampler, vec2 _coord, vec2 _invScrSize) {
    const float quality = 1.0;
    const float treshold = 0.063;
    const float tresholdMin = 0.0312;

    return fxaa_311(_sampler, _coord, _invScrSize, quality, treshold, tresholdMin);
}

void main() {
    vec4 colour = fxaa_311(s_quadtexture, gl_FragCoord.xy+u_params[0].x, u_viewTexel.xy);
    gl_FragColor = colour;
}

/* vim: set sw=4 ts=4 expandtab: */
