#define STB_IMAGE_IMPLEMENTATION
#define STB_DS_IMPLEMENTATION
#include <ktx.h>
#include <stdlib.h>
#include <engine/utils.hpp>

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

bool utils_checkktxpath(const char *filename) {
    ktxTexture *texture;
    KTX_error_code r = ktxTexture_CreateFromNamedFile(filename, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

    if (r != KTX_SUCCESS) {
        return false;
    }

    ktxTexture_Destroy(texture);
    return true;
}
