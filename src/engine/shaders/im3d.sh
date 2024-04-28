#define IM3D_ANTIALIASING 2.0

#ifdef VERTEX_SHADER

struct vertexdata {
    vec4 possize;
    uint colour;
};

layout(binding = 0) uniform ubo {
    mat4 viewproj;
    vec2 viewport;
} u_ubo;

layout(binding = 1) uniform vertexbuffer {
    vertexdata u_vertexdata[(64 * 1024) / 32];
};

layout(location = 0) in vec4 a_position;
#if defined(POINTS)
layout(location = 0) noperspective out vec2 v_texcoord;
#elif defined(LINES)
layout(location = 0) noperspective out float v_edgedistance;
#endif
layout(location = 1) noperspective out float v_size;
layout(location = 2) smooth out vec4 v_colour;

vec4 unpackuint(uint u) {
    // Unpack into RGBA (255 values each)
    vec4 ret = vec4(0.0);
    ret.r = float((u & 0xff000000u) >> 24u) / 255.0;
    ret.g = float((u & 0x00ff0000u) >> 16u) / 255.0;
    ret.b = float((u & 0x0000ff00u) >> 8u) / 255.0;
    ret.a = float((u & 0x000000ffu) >> 0u) / 255.0;
    return ret;
}

void main() {
#if defined(POINTS)
    int vtxid = gl_InstanceIndex;

    v_size = max(u_vertexdata[vtxid].possize.w, IM3D_ANTIALIASING);
    v_colour = unpackuint(u_vertexdata[vtxid].colour);
    v_colour.a *= smoothstep(0.0, 1.0, v_size / IM3D_ANTIALIASING);

    gl_Position = u_ubo.viewproj * vec4(u_vertexdata[vtxid].possize.xyz, 1.0);
    vec2 scale = 1.0 / u_ubo.viewport * v_size;
    gl_Position.xy += a_position.xy * scale * gl_Position.w;
    v_texcoord = a_position.xy * 0.5 + 0.5;
#elif defined(LINES)
    int vtxid0 = gl_InstanceIndex * 2;
    int vtxid1 = vtxid0 + 1;
    int vtxid = (gl_VertexID % 2 == 0) ? vtxid0 : vtxid1;

    v_colour = unpackuint(u_vertexdata[vtxid].colour);
    v_size = u_vertexdata[vtxid].possize.w;
    v_colour.a *= smoothstep(0.0, 1.0, v_size / IM3D_ANTIALIASING);
    v_size = max(v_size, IM3D_ANTIALIASING);
    v_edgedistance = v_size * a_position.y;

    vec4 pos0 = u_ubo.viewproj * vec4(u_vertexdata[vtxid0].possize.xyz, 1.0);
    vec4 pos1 = u_ubo.viewproj * vec4(u_vertexdata[vtxid1].possize.xyz, 1.0);
    vec2 dir = (pos0.xy / pos0.w) - (pos1.xy / pos1.w);
    dir = normalize(vec2(dir.x, dir.y - u_ubo.viewport.y / u_ubo.viewport.x));
    vec2 tng = vec2(-dir.y, dir.x) * v_size / u_ubo.viewport;

    gl_Position = (gl_VertexIndex % 2 == 0) ? pos0 : pos1;
    gl_Position.xy += tng * a_position.y * gl_Position.w;

#elif defined(TRIANGLES)
    int vtxid = gl_InstanceIndex * 3 + gl_VertexID;
    v_colour = unpackuint(u_vertexdata[vtxid].colour);
    gl_Position = u_ubo.viewproj * vec4(u_vertexdata[vtxid].possize.xyz, 1.0);
#endif
}

#endif

#ifdef FRAGMENT_SHADER

#if defined(POINTS)
layout(location = 0) noperspective in vec2 v_texcoord;
#elif defined(LINES)
layout(location = 0) noperspective in float v_edgedistance;
#endif
layout(location = 1) noperspective in float v_size;
layout(location = 2) smooth in vec4 v_colour;

layout(location = 0) out vec4 f_result;

void main() {
    f_result = v_colour;
#if defined(LINES)
    float d = abs(v_edgedistance) / v_size;
    d = smoothstep(1.0, 1.0 - (IM3D_ANTIALIASING / v_size), d);
    f_result.a *= d;
#elif defined(POINTS)
    float d = length(v_texcoord - vec2(0.5));
    d = smoothstep(0.5, 0.5 - (IM3D_ANTIALIASING / v_size), d);
    f_result.a *= d;
#endif
}
#endif
