#define STB_IMAGE_IMPLEMENTATION
#define STB_DS_IMPLEMENTATION
#include <ktx.h>
#include <stdlib.h>
#include <utils.hpp>

bgfx_shader_handle_t utils_loadshader(const char *filename) {
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    const bgfx_memory_t *mem = bgfx_alloc(size + 1);
    fread(mem->data, 1, size, f);
    mem->data[mem->size - 1] = '\0';
    fclose(f);

    return bgfx_create_shader(mem);
}

void *utils_downsize(void *data, int size, int bpp) {
    size_t mipsize = size / 2;
    uint8_t *mip = (uint8_t *)malloc(mipsize * mipsize * bpp);
    for (size_t my = 0; my < mipsize; ++my) {
        for (size_t mx = 0; mx < mipsize; ++mx) {
            for (size_t mbpp = 0; mbpp < bpp; ++mbpp) {
                int index = bpp * ((my * 2) * size + (mx * 2)) + mbpp;
                int sum = 4 >> 1;
                sum += ((uint8_t *)data)[index + bpp * (0 * size + 0)];
                sum += ((uint8_t *)data)[index + bpp * (0 * size + 1)];
                sum += ((uint8_t *)data)[index + bpp * (1 * size + 0)];
                sum += ((uint8_t *)data)[index + bpp * (1 * size + 1)];
                mip[bpp * (my * mipsize + mx) + mbpp] = (uint8_t)(sum / 4);
            }
        }
    }
    return mip;
}

void *utils_generatemips(void *fullsize, int size, int bpp, size_t *length) {
    int64_t t = utils_getcounter();
    uint8_t *mipchain = (uint8_t *)malloc(size * size * bpp);
    memcpy(mipchain, fullsize, size * size * bpp);

    size_t mipchainlen = size * size;
    size /= 2;

    while (size) {
        mipchainlen += size * size;
        mipchain = (uint8_t *)realloc(mipchain, mipchainlen * bpp); // increase mipmap chain size

        void *mip = utils_downsize(&(mipchain)[(mipchainlen - (((size * 2) * (size * 2)) + (size * size))) * bpp], size * 2, bpp); // downsize from previous mip using size of previous mip
        memcpy(mipchain + ((mipchainlen - (size * size)) * bpp), mip, size * size * bpp);
        free(mip);
        size /= 2;
    }

    *length = mipchainlen * bpp;
    int64_t t2 = utils_getcounter();

    return mipchain;
}

// TODO: DDS
bgfx_texture_handle_t utils_loadtexture(const char *filename, bool clamp) {
    stbi_set_flip_vertically_on_load(true);
    int x, y;
    int channels;
    uint8_t *data = stbi_load(filename, &x, &y, &channels, 0);

    if (data == NULL) {
        printf("failed to load texture image data\n");
        exit(1);
    }

    bgfx_texture_handle_t res;
    if (x == y) {
        size_t length = x * y * channels;
        void *mipchain = utils_generatemips(data, x, channels, &length);

        res = bgfx_create_texture_2d(x, y, true, 1, channels == 3 ? BGFX_TEXTURE_FORMAT_RGB8 : BGFX_TEXTURE_FORMAT_RGBA8, (clamp ? BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP : 0) | BGFX_SAMPLER_MAG_ANISOTROPIC | BGFX_SAMPLER_MIN_ANISOTROPIC, bgfx_copy(mipchain, length));
        free(mipchain);
    } else {
        res = bgfx_create_texture_2d(x, y, false, 1, channels == 3 ? BGFX_TEXTURE_FORMAT_RGB8 : BGFX_TEXTURE_FORMAT_RGBA8, (clamp ? BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP : 0) | BGFX_SAMPLER_MAG_ANISOTROPIC | BGFX_SAMPLER_MIN_ANISOTROPIC, bgfx_copy(data, x * y * channels));
    }

    if (res.idx == (bgfx_texture_handle_t)BGFX_INVALID_HANDLE.idx) {
        printf("texture failed to load\n");
        exit(1);
    }

    stbi_image_free(data);
    return res;
}

float *utils_loadexr(char *file) {
    // exr_context_initializer_t ctx = EXR_DEFAULT_CONTEXT_INITIALIZER;
    // exr_context_t f;
    // exr_result_t res = exr_start_read(&f, file, &ctx);
    // if (res != EXR_ERR_SUCCESS) {
    //     printf("failed to load EXR image data\n");
    //     exit(1);
    // }
    //
    // exr_attr_box2i_t window;
    // res = exr_get_data_window(f, 0, &window);
    // if (res != EXR_ERR_SUCCESS) {
    //     printf("failed to read EXR image data\n");
    //     exit(1);
    // }
    // printf("image data success!\n");
    // printf("%ux%u\n", window.max.x - window.min.x + 1, window.max.y - window.min.y + 1);
    //
    // float *data = NULL;
    // exit(0);
    return NULL;
}

bgfx_texture_handle_t utils_loadcubemapfromsides(const char *sides[], bool mips) {
    stbi_set_flip_vertically_on_load(false);

    int x, y, bpp;

    uint8_t *chain = NULL;
    size_t chainlen = 0;

    for (size_t i = BGFX_CUBE_MAP_POSITIVE_X; i < BGFX_CUBE_MAP_NEGATIVE_Z + 1; i++) {
        size_t length;
        uint8_t *data = stbi_load(sides[i], &x, &y, &bpp, 0);
        if (data == NULL) {
            printf("failed to load texture image data\n");
            exit(1);
        }
        if (mips) {

            void *mipchain = utils_generatemips(data, x, bpp, &length); // generate mipchain
            chain = (uint8_t *)realloc(chain, chainlen + length); // allocate to make space for the chain

            // copy mipchain into cubemap chain
            memcpy(chain + chainlen, mipchain, length); // copy mipchain into cubemap chain
            chainlen += length;

            free(mipchain);
        } else {
            chain = (uint8_t *)realloc(chain, chainlen + (x * y * bpp));
            memcpy(chain + chainlen, data, x * y * bpp);
            chainlen += (x * y * bpp);
        }
        stbi_image_free(data);
    }

    bgfx_texture_handle_t res = bgfx_create_texture_cube(x, mips, 1, bpp == 3 ? BGFX_TEXTURE_FORMAT_RGB8 : BGFX_TEXTURE_FORMAT_RGBA8, BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC, bgfx_copy(chain, chainlen));
    free(chain);

    if (res.idx == (bgfx_texture_handle_t)BGFX_INVALID_HANDLE.idx) {
        printf("texture failed to load\n");
        exit(1);
    }

    return res;
}

bgfx_texture_handle_t utils_loadcubemapfromktx(const char *file, bool mips) {
    ktxTexture *texture;
    KTX_error_code r = ktxTexture_CreateFromNamedFile(file, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

    if (r != KTX_SUCCESS) {
        printf("failed to load image data from KTX\n");
        exit(1);
    }
    ktx_uint8_t *data = ktxTexture_GetData(texture);

    bgfx_texture_handle_t res = bgfx_create_texture_cube(texture->baseWidth, mips ? texture->numLevels > 1 ? true : false : false, texture->numLayers, BGFX_TEXTURE_FORMAT_RGBA16F, BGFX_SAMPLER_UVW_CLAMP | BGFX_SAMPLER_POINT, bgfx_copy(data, texture->dataSize));

    ktxTexture_Destroy(texture);
    return res;
}

bool utils_checkktxpath(const char *filename) {
    ktxTexture *texture;
    KTX_error_code r = ktxTexture_CreateFromNamedFile(filename, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

    if (r != KTX_SUCCESS) {
        return false;
    }

    ktxTexture_Destroy(texture);
    return true;
}
