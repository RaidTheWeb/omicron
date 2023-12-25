#include <engine/engine.hpp>
#include <renderer/camera.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/renderer.hpp>
#include <resources/resource.hpp>
#include <utils.hpp>

size_t script_type = SCRIPT_TYPEPIPELINE;

bgfx_view_id_t v_tonemap;
bgfx_view_id_t v_histogram;
bgfx_view_id_t v_average;
bgfx_view_id_t v_light;
bgfx_view_id_t v_zpass;
bgfx_view_id_t v_aabb;
bgfx_view_id_t v_depthreduce;
bgfx_view_id_t v_skybox;
bgfx_view_id_t v_prefilter;
bgfx_view_id_t v_irradiance;
bgfx_view_id_t v_brdf;
bgfx_view_id_t v_post;

struct resource_gpu *p_average;
struct resource_gpu *p_brdfLUT;
struct resource_gpu *p_cluster;
struct resource_gpu *p_culllights;
struct resource_gpu *p_depthprepass;
struct resource_gpu *p_fxaa;
struct resource_gpu *p_histogram;
struct resource_gpu *p_irradiance;
struct resource_gpu *p_pbrclustered;
struct resource_gpu *p_prefilter;
struct resource_gpu *p_skybox;
struct resource_gpu *p_tonemap;

// static void framebuffers(struct pipeline *pipeline, struct renderer *renderer, size_t flags) {
//     if (flags & RENDERER_RENDERFLAGRECTUPDATED) {
//         printf("resize\n");
//         struct resource_gpu *f_hdr = renderer_getlocalstorage(renderer->local, utils_stringid("f_hdr"));
//         struct resource_gpu *t_hdr0 = renderer_getlocalstorage(renderer->local, utils_stringid("t_hdr0"));
//         struct resource_gpu *t_hdr1 = renderer_getlocalstorage(renderer->local, utils_stringid("t_hdr1"));
//         struct resource_gpu *f_post = renderer_getlocalstorage(renderer->local, utils_stringid("f_post"));
//         struct resource_gpu *t_post = renderer_getlocalstorage(renderer->local, utils_stringid("t_post"));
//         RESOURCE_MARKFREE(f_hdr);
//         RESOURCE_MARKFREE(t_hdr0);
//         RESOURCE_MARKFREE(t_hdr1);
//         RESOURCE_MARKFREE(f_post);
//         RESOURCE_MARKFREE(t_post);
//     } else if (flags & RENDERER_RENDERFLAGFIRST) {
//         struct resource_gpu *t_lumavg = RESOURCE_INIT(bgfx_create_texture_2d(128, 128, false, 1, BGFX_TEXTURE_FORMAT_R16F, BGFX_SAMPLER_UVW_CLAMP | BGFX_SAMPLER_POINT, NULL), GPU_TEXTURE);
//         struct resource_gpu *t_brdflut = RESOURCE_INIT(bgfx_create_texture_2d(128, 128, false, 1, BGFX_TEXTURE_FORMAT_RG16F, BGFX_TEXTURE_COMPUTE_WRITE | BGFX_SAMPLER_UVW_CLAMP | BGFX_SAMPLER_POINT, NULL), GPU_TEXTURE);
//
//         renderer_setlocalstorage(renderer->local, utils_stringid("t_lumavg"), t_lumavg);
//         renderer_setlocalstorage(renderer->local, utils_stringid("t_brdflut"), t_brdflut);
//     }
//
//     printf("updating renderer winsize to %f %f\n", renderer->camera->rect.x, renderer->camera->rect.y);
//     struct resource_gpu *t_hdr0 = RESOURCE_INIT(bgfx_create_texture_2d(renderer->camera->rect.x, renderer->camera->rect.y, false, 1, BGFX_TEXTURE_FORMAT_RGBA16F, BGFX_SAMPLER_POINT | BGFX_SAMPLER_UVW_CLAMP | BGFX_TEXTURE_RT, NULL), GPU_TEXTURE);
//     struct resource_gpu *t_hdr1 = RESOURCE_INIT(bgfx_create_texture_2d(renderer->camera->rect.x, renderer->camera->rect.y, false, 1, BGFX_TEXTURE_FORMAT_D24S8, BGFX_SAMPLER_POINT | BGFX_SAMPLER_UVW_CLAMP | BGFX_TEXTURE_RT_WRITE_ONLY, NULL), GPU_TEXTURE);
//     bgfx_attachment_t hdrattachments[2] = { 0 };
//     bgfx_attachment_init(&hdrattachments[0], t_hdr0->texture, BGFX_ACCESS_WRITE, 0, 1, 0, BGFX_RESOLVE_AUTO_GEN_MIPS);
//     bgfx_attachment_init(&hdrattachments[1], t_hdr1->texture, BGFX_ACCESS_WRITE, 0, 1, 0, BGFX_RESOLVE_AUTO_GEN_MIPS);
//     struct resource_gpu *f_hdr = RESOURCE_INIT(bgfx_create_frame_buffer_from_attachment(2, hdrattachments, false), GPU_FRAMEBUFFER);
//
//     renderer_setlocalstorage(renderer->local, utils_stringid("f_hdr"), f_hdr);
//     renderer_setlocalstorage(renderer->local, utils_stringid("t_hdr0"), t_hdr0);
//     renderer_setlocalstorage(renderer->local, utils_stringid("t_hdr1"), t_hdr1);
//     struct resource_gpu *t_post = RESOURCE_INIT(bgfx_create_texture_2d(renderer->camera->rect.x, renderer->camera->rect.y, false, 1, BGFX_TEXTURE_FORMAT_RGBA16F, BGFX_SAMPLER_POINT | BGFX_SAMPLER_UVW_CLAMP | BGFX_TEXTURE_RT, NULL), GPU_TEXTURE);
//     bgfx_attachment_t postattachment;
//     bgfx_attachment_init(&postattachment, t_post->texture, BGFX_ACCESS_WRITE, 0, 1, 0, BGFX_RESOLVE_AUTO_GEN_MIPS);
//     struct resource_gpu *f_post = RESOURCE_INIT(bgfx_create_frame_buffer_from_attachment(1, &postattachment, false), GPU_FRAMEBUFFER);
//
//     renderer_setlocalstorage(renderer->local, utils_stringid("f_post"), f_post);
//     renderer_setlocalstorage(renderer->local, utils_stringid("t_post"), t_post);
// }
//
// static void computebuffers(struct pipeline *pipeline, struct renderer *renderer, size_t flags) {
//     if (flags & RENDERER_RENDERFLAGRECTUPDATED) {
//         struct resource_gpu *b_clustermin = renderer_getlocalstorage(renderer->local, utils_stringid("b_clustermin"));
//         struct resource_gpu *b_clustermax = renderer_getlocalstorage(renderer->local, utils_stringid("b_clustermax"));
//         RESOURCE_MARKFREE(b_clustermin);
//         RESOURCE_MARKFREE(b_clustermax);
//     } else if (flags & RENDERER_RENDERFLAGFIRST) {
//         struct resource_gpu *b_histogram = RESOURCE_INIT(bgfx_create_dynamic_index_buffer(256, BGFX_BUFFER_COMPUTE_READ_WRITE | BGFX_BUFFER_INDEX32), GPU_DYNAMIC_INDEX);
//         renderer_setlocalstorage(renderer->local, utils_stringid("b_histogram"), b_histogram);
//     }
//
//     struct camera *camera = renderer->camera;
//
//     vec2s ar = (vec2s) { camera->rect.x / utils_gcd(camera->rect.x, camera->rect.y), camera->rect.y / utils_gcd(camera->rect.x, camera->rect.y) }; // aspect ratio (smallest value)
//     vec4s params = (vec4s) { ar.x, ar.y, 24.0f, ceilf(camera->rect.x / ar.x) };
//     struct resource_gpu *b_clustermin = RESOURCE_INIT(bgfx_create_dynamic_index_buffer(params.x * params.y * params.z * sizeof(vec4s), BGFX_BUFFER_COMPUTE_READ_WRITE | BGFX_BUFFER_INDEX32), GPU_DYNAMIC_INDEX);
//     struct resource_gpu *b_clustermax = RESOURCE_INIT(bgfx_create_dynamic_index_buffer(params.x * params.y * params.z * sizeof(vec4s), BGFX_BUFFER_COMPUTE_READ_WRITE | BGFX_BUFFER_INDEX32 | BGFX_BUFFER_COMPUTE_FORMAT_32X4), GPU_DYNAMIC_INDEX);
//     renderer_setlocalstorage(renderer->local, utils_stringid("b_clustermin"), b_clustermin);
//     renderer_setlocalstorage(renderer->local, utils_stringid("b_clustermax"), b_clustermax);
//     struct resource_gpu *u_clusterparams = pipeline_getglobalstorage(pipeline->global, utils_stringid("u_clusterparams"));
//     bgfx_set_uniform(u_clusterparams->uniform, ((vec4s[]) { (vec4s) { camera->near, camera->far, camera->rect.x, camera->rect.y }, params }), 2);
//     bgfx_set_compute_dynamic_index_buffer(0, b_clustermin->dindex, BGFX_ACCESS_WRITE);
//     bgfx_set_compute_dynamic_index_buffer(1, b_clustermax->dindex, BGFX_ACCESS_WRITE);
//     bgfx_dispatch(v_aabb, p_cluster->program, params.x, params.y, params.z, BGFX_DISCARD_ALL);
// }
//
// static void uniforms(struct pipeline *pipeline) {
//     struct resource_gpu *s_avglum = RESOURCE_INIT(bgfx_create_uniform("s_avglum", BGFX_UNIFORM_TYPE_SAMPLER, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("s_avglum"), s_avglum);
//     struct resource_gpu *s_brdflut = RESOURCE_INIT(bgfx_create_uniform("s_brdflut", BGFX_UNIFORM_TYPE_SAMPLER, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("s_brdflut"), s_brdflut);
//     struct resource_gpu *s_depth = RESOURCE_INIT(bgfx_create_uniform("s_depth", BGFX_UNIFORM_TYPE_SAMPLER, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("s_depth"), s_depth);
//     struct resource_gpu *s_env_irradiance = RESOURCE_INIT(bgfx_create_uniform("s_env_irradiance", BGFX_UNIFORM_TYPE_SAMPLER, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("s_env_irradiance"), s_env_irradiance);
//     struct resource_gpu *s_env_prefiltered = RESOURCE_INIT(bgfx_create_uniform("s_env_prefiltered", BGFX_UNIFORM_TYPE_SAMPLER, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("s_env_prefiltered"), s_env_prefiltered);
//     struct resource_gpu *s_quadtexture = RESOURCE_INIT(bgfx_create_uniform("s_quadtexture", BGFX_UNIFORM_TYPE_SAMPLER, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("s_quadtexture"), s_quadtexture);
//     struct resource_gpu *s_texture_base = RESOURCE_INIT(bgfx_create_uniform("s_texture_base", BGFX_UNIFORM_TYPE_SAMPLER, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("s_texture_base"), s_texture_base);
//     struct resource_gpu *s_texture_normal = RESOURCE_INIT(bgfx_create_uniform("s_texture_normal", BGFX_UNIFORM_TYPE_SAMPLER, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("s_texture_normal"), s_texture_normal);
//     struct resource_gpu *s_texture_metallicroughness = RESOURCE_INIT(bgfx_create_uniform("s_texture_metallicroughness", BGFX_UNIFORM_TYPE_SAMPLER, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("s_texture_metallicroughness"), s_texture_metallicroughness);
//     struct resource_gpu *s_texture_emissive = RESOURCE_INIT(bgfx_create_uniform("s_texture_emissive", BGFX_UNIFORM_TYPE_SAMPLER, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("s_texture_emissive"), s_texture_emissive);
//     struct resource_gpu *s_texture_occlusion = RESOURCE_INIT(bgfx_create_uniform("s_texture_occlusion", BGFX_UNIFORM_TYPE_SAMPLER, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("s_texture_occlusion"), s_texture_occlusion);
//     struct resource_gpu *s_env_irradiancecubemap = RESOURCE_INIT(bgfx_create_uniform("s_env_irradiancecubemap", BGFX_UNIFORM_TYPE_SAMPLER, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("s_env_irradiancecubemap"), s_env_irradiancecubemap);
//     struct resource_gpu *s_env_prefiltercubemap = RESOURCE_INIT(bgfx_create_uniform("s_env_prefiltercubemap", BGFX_UNIFORM_TYPE_SAMPLER, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("s_env_prefiltercubemap"), s_env_prefiltercubemap);
//
//     struct resource_gpu *u_campos = RESOURCE_INIT(bgfx_create_uniform("u_campos", BGFX_UNIFORM_TYPE_VEC4, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("u_campos"), u_campos);
//     struct resource_gpu *u_invrotationviewproj = RESOURCE_INIT(bgfx_create_uniform("u_invrotationviewproj", BGFX_UNIFORM_TYPE_MAT4, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("u_invrotationviewproj"), u_invrotationviewproj);
//     struct resource_gpu *u_clusterparams = RESOURCE_INIT(bgfx_create_uniform("u_clusterparams", BGFX_UNIFORM_TYPE_VEC4, 2), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("u_clusterparams"), u_clusterparams);
//     struct resource_gpu *u_histparams = RESOURCE_INIT(bgfx_create_uniform("u_histparams", BGFX_UNIFORM_TYPE_VEC4, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("u_histparams"), u_histparams);
//     struct resource_gpu *u_avgparams = RESOURCE_INIT(bgfx_create_uniform("u_avgparams", BGFX_UNIFORM_TYPE_VEC4, 1), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("u_avgparams"), u_avgparams);
//     struct resource_gpu *u_skyboxparams = RESOURCE_INIT(bgfx_create_uniform("u_skyboxparams", BGFX_UNIFORM_TYPE_VEC4, 5), GPU_UNIFORM);
//     pipeline_setglobalstorage(pipeline->global, utils_stringid("u_skyboxparams"), u_skyboxparams);
// }
//
// static void views(struct pipeline *pipeline) {
//
//     // v_editor = view++;
//     // bgfx_set_view_name(v_editor, "Editor");
//     // bgfx_set_view_clear(v_editor, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0.0f);
//     // bgfx_set_view_mode(v_editor, BGFX_VIEW_MODE_SEQUENTIAL);
//
//     v_post = pipeline_view(pipeline);
//     bgfx_set_view_name(v_post, "Post Processing");
//     bgfx_set_view_clear(v_post, 0 | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0.0f);
//     bgfx_set_view_mode(v_post, BGFX_VIEW_MODE_SEQUENTIAL);
//
//     v_average = pipeline_view(pipeline);
//     bgfx_set_view_name(v_average, "Luminescence Averaging");
//
//     v_histogram = pipeline_view(pipeline);
//     bgfx_set_view_name(v_histogram, "Luminescence Histogram");
//
//     v_skybox = pipeline_view(pipeline);
//     bgfx_set_view_name(v_skybox, "Skybox");
//
//     v_light = pipeline_view(pipeline);
//     bgfx_set_view_name(v_light, "Draw Primitives + Lighting Pass");
//
//     v_zpass = pipeline_view(pipeline);
//     bgfx_set_view_name(v_zpass, "Z Prepass");
//     bgfx_set_view_clear(v_zpass, 0 | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0.0f);
//
//     v_aabb = pipeline_view(pipeline);
//     bgfx_set_view_name(v_aabb, "Cluster AABB Calculation");
//
//     v_prefilter = pipeline_view(pipeline);
//     bgfx_set_view_name(v_prefilter, "Prefiltered Cubemap Calculation");
//
//     v_irradiance = pipeline_view(pipeline);
//     bgfx_set_view_name(v_irradiance, "Irradiance Map Calculation");
//
//     v_brdf = pipeline_view(pipeline);
//     bgfx_set_view_name(v_brdf, "BRDF LUT Calculation");
//
//     pipeline_vieworder(pipeline);
// }
//
// static void programs(struct pipeline *pipeline) {
//     p_average = RESOURCE_INIT(bgfx_create_compute_program(utils_loadshader("assets/shaders/lum/cs_average.bin"), true), GPU_PROGRAM);
//     p_brdfLUT = RESOURCE_INIT(bgfx_create_compute_program(utils_loadshader("assets/shaders/lum/cs_brdfLUT.bin"), true), GPU_PROGRAM);
//     p_cluster = RESOURCE_INIT(bgfx_create_compute_program(utils_loadshader("assets/shaders/clustered/cs_cluster.bin"), true), GPU_PROGRAM);
//     p_culllights = RESOURCE_INIT(bgfx_create_compute_program(utils_loadshader("assets/shaders/clustered/cs_lightcull.bin"), true), GPU_PROGRAM);
//     p_depthprepass = RESOURCE_INIT(bgfx_create_program(utils_loadshader("assets/shaders/optimisations/vs_zprepass.bin"), utils_loadshader("assets/shaders/optimisations/fs_zprepass.bin"), true), GPU_PROGRAM);
//     p_fxaa = RESOURCE_INIT(bgfx_create_program(utils_loadshader("assets/shaders/post/vs_fxaa.bin"), utils_loadshader("assets/shaders/post/fs_fxaa.bin"), true), GPU_PROGRAM);
//     p_histogram = RESOURCE_INIT(bgfx_create_compute_program(utils_loadshader("assets/shaders/lum/cs_histogram.bin"), true), GPU_PROGRAM);
//     p_irradiance = RESOURCE_INIT(bgfx_create_compute_program(utils_loadshader("assets/shaders/lum/cs_irradiance.bin"), true), GPU_PROGRAM);
//     p_pbrclustered = RESOURCE_INIT(bgfx_create_program(utils_loadshader("assets/shaders/clustered/vs_pbr.bin"), utils_loadshader("assets/shaders/clustered/fs_pbr.bin"), true), GPU_PROGRAM);
//     p_prefilter = RESOURCE_INIT(bgfx_create_compute_program(utils_loadshader("assets/shaders/lum/cs_prefilter.bin"), true), GPU_PROGRAM);
//     p_skybox = RESOURCE_INIT(bgfx_create_program(utils_loadshader("assets/shaders/clustered/vs_skybox.bin"), utils_loadshader("assets/shaders/clustered/fs_skybox.bin"), true), GPU_PROGRAM);
//     p_tonemap = RESOURCE_INIT(bgfx_create_program(utils_loadshader("assets/shaders/post/vs_tonemap.bin"), utils_loadshader("assets/shaders/post/fs_tonemap.bin"), true), GPU_PROGRAM);
//
// }
//
// bgfx_vertex_layout_t layout;
// bgfx_vertex_buffer_handle_t vb;
// bgfx_index_buffer_handle_t ib;
//
// // flip vertical, not horizontally
// float quadvertices[] = {
//     -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
//     -1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
//     1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
//     1.0f, -1.0f, 0.0f, 1.0f, 1.0f
// };
//
// // more or less normal
// float flipquadvertices[] = {
//     -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
//     -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
//     1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
//     1.0f, -1.0f, 0.0f, 1.0f, 0.0f
// };
//
// uint16_t quadindices[] = {
//     0, 2, 1,
//     0, 3, 2
// };
//
// bgfx_texture_handle_t skybox;
// void pipeline_setup(struct pipeline *pipeline) {
//     uniforms(pipeline);
//     views(pipeline);
//     programs(pipeline);
//
//     skybox = utils_loadcubemapfromktx("/home/rtw/Desktop/output_skybox2.ktx2", false);
//
//     bgfx_vertex_layout_begin(&layout, bgfx_get_renderer_type());
//     bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
//     bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_TEXCOORD0, 2, BGFX_ATTRIB_TYPE_FLOAT, false, false);
//     bgfx_vertex_layout_end(&layout);
//     vb = bgfx_create_vertex_buffer(bgfx_make_ref(bgfx_get_caps()->originBottomLeft ? flipquadvertices : quadvertices, sizeof(quadvertices)), &layout, BGFX_BUFFER_NONE);
//     ib = bgfx_create_index_buffer(bgfx_make_ref(quadindices, sizeof(quadindices)), BGFX_BUFFER_NONE);
// }
//

bgfx_vertex_layout_t layout;
bgfx_vertex_buffer_handle_t vb;
bgfx_index_buffer_handle_t ib;

// flip vertical, not horizontally
float quadvertices[] = {
    -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f, 1.0f, 1.0f
};

// more or less normal
float flipquadvertices[] = {
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
    -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    1.0f, -1.0f, 0.0f, 1.0f, 0.0f
};

uint16_t quadindices[] = {
    0, 2, 1,
    0, 3, 2
};

bgfx_texture_handle_t skybox;


static void renderscreenquad(bgfx_view_id_t view, bgfx_program_handle_t program, bool renderback) {
    // render_settexture(program, 0, "s_quadtexture", handle, UINT32_MAX);
    bgfx_set_vertex_buffer(0, vb, 0, UINT32_MAX);
    bgfx_set_index_buffer(ib, 0, UINT32_MAX);
    bgfx_set_state(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | (renderback ? BGFX_STATE_DEPTH_TEST_LEQUAL : 0), 0);
    bgfx_submit(view, program, 0, BGFX_DISCARD_ALL);
}
//
// #include <engine/matrix.h>
//
// void pipeline_render(struct pipeline *pipeline, struct renderer *renderer, size_t flags) {
//     // flags:
//     // first render
//     // view updated
//     // running under editor
//     if (flags & RENDERER_RENDERFLAGFIRST || flags & RENDERER_RENDERFLAGRECTUPDATED) {
//         computebuffers(pipeline, renderer, flags);
//         framebuffers(pipeline, renderer, flags);
//         bgfx_set_view_rect(v_post, 0, 0, renderer->camera->rect.x, renderer->camera->rect.y);
//         bgfx_set_view_rect(v_skybox, 0, 0, renderer->camera->rect.x, renderer->camera->rect.y);
//         bgfx_set_view_rect(v_light, 0, 0, renderer->camera->rect.x, renderer->camera->rect.y);
//         bgfx_set_view_rect(v_zpass, 0, 0, renderer->camera->rect.x, renderer->camera->rect.y);
//     }
//
//     // up and down is swapped
//     struct camera *camera = renderer->camera;
//     mat4s vc;
//     memcpy(&vc, &camera->view, sizeof(mat4s));
//     ((float *)&vc)[12] = 0.0f;
//     ((float *)&vc)[13] = 0.0f;
//     ((float *)&vc)[14] = 0.0f;
//
//     struct resource_gpu *u_skyboxparams = pipeline_getglobalstorage(pipeline->global, utils_stringid("u_skyboxparams"));
//     struct resource_gpu *s_quadtexture = pipeline_getglobalstorage(pipeline->global, utils_stringid("s_quadtexture"));
//
//     mat4s vmtx = glms_mat4_inv(vc);
//     bgfx_set_uniform(u_skyboxparams->uniform, ((vec4s[]) { (vec4s) { camera->fov, 0.0f, 0.0f, 0.0f }, vmtx.col[0], vmtx.col[1], vmtx.col[2], vmtx.col[3] }), 5);
//     bgfx_set_texture(0, s_quadtexture->uniform, skybox, UINT32_MAX);
//     // mat4s ortho = glms_ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 2.0f);
//     // bgfx_set_view_transform(pipeline_vmain, &camera->view, NULL);
//     // mtx_ortho((float *)&ortho, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f, bgfx_get_caps()->homogeneousDepth);
//     renderscreenquad(pipeline_vmain, p_skybox->program, true);
//
//     // struct game_object **visible = renderer_visible(renderer);
//     // shadow pass
//     // z prepass
//     // lighting pass
//     // decals
//     // water pass
//     // transparency pass
//     // transparency decals (?)
//     // post processing
//     // tone mapping
//     // UI
//     // anti-aliasing
//     //
//     // editor defaults (required but only when in editor preview mode)
// }

void pipeline_mainviewresize(struct pipeline_view *view, vec2s size) {
    bgfx_set_view_rect(1, 0, 0, size.x, size.y);
}

void pipeline_mainviewrender(struct pipeline_view *view, size_t flags) {
    pipeline_renderpass(view, "MainMain");
}

bgfx_uniform_handle_t u_skyboxparams;
bgfx_uniform_handle_t s_quadtexture;

void pipeline_mainpassrender(struct pipeline_pass *pass) {
    struct camera *camera = pass->view->renderer->camera;
    mat4s vc;
    memcpy(&vc, &camera->view, sizeof(mat4s)); // hecking breaks???????????? but why, pls let me know bbg
    ((float *)vc.raw)[12] = 0.0f;
    ((float *)vc.raw)[13] = 0.0f;
    ((float *)vc.raw)[14] = 0.0f;

    mat4s vmtx = glms_mat4_inv(vc);
    bgfx_set_uniform(u_skyboxparams, ((vec4s[]) { (vec4s) { camera->fov, utils_getcounter(), 0.0f, 0.0f }, vmtx.col[0], vmtx.col[1], vmtx.col[2], vmtx.col[3] }), 5);
    bgfx_set_texture(0, s_quadtexture, skybox, UINT32_MAX);
    // mat4s ortho = glms_ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 2.0f);
    // bgfx_set_view_transform(pass->id, &vc, NULL);
    // mtx_ortho((float *)&ortho, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f, bgfx_get_caps()->homogeneousDepth);
    renderscreenquad(pass->id, p_skybox->program, true);
}

int script_init(struct pipeline *pipeline) {
    struct pipeline_view *mainview = malloc(sizeof(struct pipeline_view));
    memset(mainview, 0, sizeof(struct pipeline_view));
    pipeline_initview(pipeline, mainview, 2);
    mainview->name = "Main";
    mainview->sourcetype = PIPELINE_VIEWSOURECCAMERA;
    mainview->camera = engine_engine->activecam;
    mainview->resize = pipeline_mainviewresize;
    mainview->render = pipeline_mainviewrender;
    engine_engine->activecam->pos = (vec3s) { 0.0f, 1.5f, 0.0f };

    skybox = utils_loadcubemapfromktx("/home/rtw/Desktop/output_skybox2.ktx2", false);

    // make sure to initialise uniforms before initialising shaders!!!!
    // how should uniforms be set up? hard coded and the pipeline script just requests them from the resource manager? or should it be in pipeline storage, or even in view storage?
    s_quadtexture = bgfx_create_uniform("s_quadtexture", BGFX_UNIFORM_TYPE_SAMPLER, 1);
    u_skyboxparams = bgfx_create_uniform("u_skyboxparams", BGFX_UNIFORM_TYPE_VEC4, 5);

    p_skybox = RESOURCE_INIT(bgfx_create_program(utils_loadshader("assets/shaders/clustered/vs_skybox.bin"), utils_loadshader("assets/shaders/clustered/fs_skybox.bin"), true), GPU_PROGRAM);

    bgfx_vertex_layout_begin(&layout, bgfx_get_renderer_type());
    bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_TEXCOORD0, 2, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_end(&layout);
    vb = bgfx_create_vertex_buffer(bgfx_make_ref(bgfx_get_caps()->originBottomLeft ? flipquadvertices : quadvertices, sizeof(quadvertices)), &layout, BGFX_BUFFER_NONE);
    ib = bgfx_create_index_buffer(bgfx_make_ref(quadindices, sizeof(quadindices)), BGFX_BUFFER_NONE);

    struct pipeline_pass *mainpass = &mainview->passes[0];
    pipeline_initpass(pipeline, mainpass);
    mainpass->name = "MainMain";
    if (true) {
        mainpass->clearcolour = (vec4s) { 0.0f, 0.0f, 0.0f, 1.0f };
        mainpass->cleardepth = 1.0f;
        mainpass->clearstencil = 0;
        bgfx_set_view_clear(1, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
    } else {
        // TODO: Editor
    }
    mainpass->render = pipeline_mainpassrender;

    pipeline_registerview(pipeline, mainview);

    return 0;
}
