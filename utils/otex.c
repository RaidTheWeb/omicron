#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ktx.h>

#define ASSERT(cond, ...) ({ \
    if (!(cond)) { \
        printf("Assertion (" #cond ") failed in %s, function %s on line %d:\n\t", __FILE__, __FUNCTION__, __LINE__); \
        printf(__VA_ARGS__); \
        abort(); \
    } \
})

enum {
    // "": UNORM, "S": SNORM, "I": SINT, "U": UINT, "F": FLOAT
    FORMAT_R8,
    FORMAT_R8I,
    FORMAT_R8U,
    FORMAT_R8S,
    FORMAT_R8SRGB,

    FORMAT_R16,
    FORMAT_R16I,
    FORMAT_R16U,
    FORMAT_R16F,
    FORMAT_R16S,

    FORMAT_R32I,
    FORMAT_R32U,
    FORMAT_R32F,

    FORMAT_RG8,
    FORMAT_RG8I,
    FORMAT_RG8U,
    FORMAT_RG8S,
    FORMAT_RG8SRGB,

    FORMAT_RG16,
    FORMAT_RG16I,
    FORMAT_RG16U,
    FORMAT_RG16F,
    FORMAT_RG16S,

    FORMAT_RG32F,

    FORMAT_RGB8,
    FORMAT_RGB8I,
    FORMAT_RGB8U,
    FORMAT_RGB8S,
    FORMAT_RGB8SRGB,

    FORMAT_RGB16,
    FORMAT_RGB16I,
    FORMAT_RGB16U,
    FORMAT_RGB16F,
    FORMAT_RGB16S,

    FORMAT_RGB32F,

    FORMAT_BGRA8,
    FORMAT_BGRA8I,
    FORMAT_BGRA8U,
    FORMAT_BGRA8S,
    FORMAT_BGRA8SRGB,

    FORMAT_RGBA8,
    FORMAT_RGBA8I,
    FORMAT_RGBA8U,
    FORMAT_RGBA8S,
    FORMAT_RGBA8SRGB,

    FORMAT_RGBA16,
    FORMAT_RGBA16I,
    FORMAT_RGBA16U,
    FORMAT_RGBA16F,
    FORMAT_RGBA16S,

    FORMAT_RGBA32I,
    FORMAT_RGBA32U,
    FORMAT_RGBA32F,

    FORMAT_B5G6R5,
    FORMAT_R5G6B5,

    FORMAT_BGRA4,
    FORMAT_RGBA4,

    FORMAT_BGR5A1,
    FORMAT_RGB5A1,

    FORMAT_RGB10A2,

    FORMAT_D16,
    FORMAT_D32F,
    FORMAT_D0S8U,
    FORMAT_D16S8U,
    FORMAT_D24S8U,
    FORMAT_D32FS8U,

    FORMAT_COUNT
};


struct header {
    char magic[5]; // OTEX\0
    uint32_t type; // Image type (2d, 3d, etc.)
    uint32_t format; // Generic format for the image.
    uint32_t width; // Width (assumed to be the lowest mip level)
    uint32_t height; // Height (assumed to be the lowest mip level)
    uint32_t depth; // Depth
    uint32_t facecount;
    uint32_t layercount; // Number of layers in the image (or faces on a cubemap).
    uint32_t levelcount; // Mipmap levels
    uint32_t compression; // Compression enabled? (and what kind? supercompression? CPU-only?)
};

struct levelhdr {
    size_t offset; // Offset of mip level data within the file
    size_t size; // Size of texture data (will be the compressed size if the texture is marked to use host or GPU compression)
    size_t uncompressedsize; // Will represent the original data size (differing from the compressed data size) only *if* the texture is marked to use host compression or supercompression
    uint32_t level;
};

struct work  {
    struct header *header;
    size_t *offset;
    FILE *f;
};

// Every mip level stores its internal byte data formatted by encapsulating layers, faces, and pixels in a tightly packed format.
// layers
//      - faces
//          - depth
//              - raw pixel data
// This layout transfers over to the GPU in about the same layout. Thanks to each level being separated this way, a single mip upload through texture streaming will upload the layers, faces, and pixels over in one upload.

static uint8_t convertformat(uint32_t vkformat) {
    switch (vkformat) {
        // Vulkan to our render format (since KTX2 provides us a vulkan format member already!)
        case 2: return FORMAT_RGBA4;
        case 3: return FORMAT_BGRA4;
        case 4: return FORMAT_R5G6B5;
        case 5: return FORMAT_B5G6R5;
        case 6: return FORMAT_RGB5A1;
        case 7: return FORMAT_BGR5A1;
        case 9: return FORMAT_R8;
        case 10: return FORMAT_R8S;
        case 13: return FORMAT_R8U;
        case 14: return FORMAT_R8I;
        case 15: return FORMAT_R8SRGB;
        case 16: return FORMAT_RG8;
        case 17: return FORMAT_RG8S;
        case 20: return FORMAT_RG8U;
        case 21: return FORMAT_RG8I;
        case 22: return FORMAT_RG8SRGB;
        case 23: return FORMAT_RGB8;
        case 24: return FORMAT_RGB8S;
        case 27: return FORMAT_RGB8U;
        case 28: return FORMAT_RGB8I;
        case 29: return FORMAT_RGB8SRGB;
        case 37: return FORMAT_RGBA8;
        case 38: return FORMAT_RGBA8S;
        case 41: return FORMAT_RGBA8U;
        case 42: return FORMAT_RGBA8I;
        case 43: return FORMAT_RGBA8SRGB;
        case 44: return FORMAT_BGRA8U;
        case 45: return FORMAT_BGRA8S;
        case 48: return FORMAT_BGRA8U;
        case 49: return FORMAT_BGRA8I;
        case 50: return FORMAT_BGRA8SRGB;
        case 58: return FORMAT_RGB10A2;
        case 70: return FORMAT_R16;
        case 71: return FORMAT_R16S;
        case 74: return FORMAT_R16U;
        case 75: return FORMAT_R16I;
        case 76: return FORMAT_R16F;
        case 77: return FORMAT_RG16;
        case 78: return FORMAT_RG16S;
        case 81: return FORMAT_RG16U;
        case 82: return FORMAT_RG16I;
        case 83: return FORMAT_RG16F;
        case 84: return FORMAT_RGB16;
        case 85: return FORMAT_RGB16S;
        case 88: return FORMAT_RGB16U;
        case 89: return FORMAT_RGB16I;
        case 90: return FORMAT_RGB16F;
        case 91: return FORMAT_RGBA16;
        case 92: return FORMAT_RGBA16S;
        case 95: return FORMAT_RGBA16U;
        case 96: return FORMAT_RGBA16I;
        case 97: return FORMAT_RGBA16F;
        case 98: return FORMAT_R32U;
        case 99: return FORMAT_R32I;
        case 100: return FORMAT_R32F;
        case 103: return FORMAT_RG32F;
        case 106: return FORMAT_RGB32F;
        case 107: return FORMAT_RGBA32U;
        case 108: return FORMAT_RGBA32I;
        case 109: return FORMAT_RGBA32F;
        case 124: return FORMAT_D16;
        case 126: return FORMAT_D32F;
        case 127: return FORMAT_D0S8U;
        case 128: return FORMAT_D16S8U;
        case 129: return FORMAT_D24S8U;
        case 130: return FORMAT_D32FS8U;
        default: return FORMAT_COUNT;
    }
}

enum {
    IMAGETYPE_1D,
    IMAGETYPE_2D,
    IMAGETYPE_3D,
    IMAGETYPE_CUBE,
    IMAGETYPE_1DARRAY,
    IMAGETYPE_2DARRAY,
    IMAGETYPE_CUBEARRAY,
    IMAGETYPE_COUNT
};



static bool endswith(const char* str, const char* suffix) {
    ASSERT(str != NULL && suffix != NULL, "Invalid string or suffic.\n");

    const size_t len = strlen(str);
    const size_t suffixlen = strlen(suffix);

    if (suffixlen > len) {
        return false; // Implicitly false as a string cannot end with something it isn't even long enough to contain.
    }
    return !strncmp(str + len - suffixlen, suffix, suffixlen);
}

static void iteratektx2(uint32_t mip, uint32_t face, uint32_t width, uint32_t height, uint32_t depth, ktx_uint64_t facelodsize, void *pixels, struct work *work) {
    struct levelhdr header = {
        .offset = *work->offset,
        .size = facelodsize, // Assuming that the data is as it says it is.
        .uncompressedsize = facelodsize, // XXX: No compression supported yet.
        .level = mip
    };
    *work->offset += facelodsize;
    ASSERT(fwrite(&header, sizeof(struct levelhdr), 1, work->f) > 0, "Failed to write mip level header.\n");

    // Here we store the current location (in the headers array) and seek to where our allocated offset is before writing the data and returning.
    size_t offset = ftell(work->f); // Save point.
    ASSERT(!fseek(work->f, header.offset, SEEK_SET), "Failed to seek to location for data.\n");
    ASSERT(fwrite(pixels, facelodsize, 1, work->f) > 0, "Failed to write level data.\n");
    ASSERT(!fseek(work->f, offset, SEEK_SET), "Failed to restore position before seek.\n");
}

void convertktx2(const char *path, const char *out) {
    ktxTexture *texture = NULL;
    KTX_error_code res = ktxTexture_CreateFromNamedFile(path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
    ASSERT(res == KTX_SUCCESS, "Failed to load KTX2 texture from file %u.\n", res);

    ktx_uint8_t *data = ktxTexture_GetData(texture); // Gather the data of the entire file.
    ASSERT(data != NULL, "Failed to acquire KTX2 texture data.\n");

    // XXX: Sort through these based on what we can get through ktxTexture_GetImageOffset
    // Because the file format is designed to be so similar *to* KTX2 we can just as easily yank all the data level by level, but for other encoded formats it won't be so easy.
    // The reason we use our own format in the engine instead of KTX2 is because libktx (and by extension the format itself) *requires* loading the entire file every time you wish to grab data out of it. Our format means you can index any layer from just an open file descriptor (thereby bypassing the requirement for more memory to load *all* data).

    FILE *f = fopen(out, "w");
    ASSERT(f != NULL, "Failed to open OTexture output.\n");

    struct header header = { };

    ASSERT(texture->numDimensions == 3 ? !texture->isArray : true, "3D images as an array is not supported.\n");
    header.type = texture->isCubemap ?
        texture->isArray ? IMAGETYPE_CUBEARRAY : IMAGETYPE_CUBE :
        texture->numDimensions == 1 ?
            texture->isArray ? IMAGETYPE_1DARRAY : IMAGETYPE_1D :
        texture->numDimensions == 2 ?
            texture->isArray ? IMAGETYPE_2DARRAY : IMAGETYPE_2D :
        texture->numDimensions == 3 ? IMAGETYPE_3D : UINT8_MAX;

    header.width = texture->baseWidth;
    header.height = texture->baseHeight;
    header.depth = texture->baseDepth;
    header.levelcount = texture->numLevels;
    header.layercount = texture->numLayers;
    header.facecount = texture->numFaces;
    header.compression = 0; // XXX: No compression supported for now.
    header.format = convertformat(((ktxTexture2 *)texture)->vkFormat); // Convert KTX2 format to our format system.
    strcpy(header.magic, "OTEX"); // Insert header magic

    ASSERT(fwrite(&header, sizeof(struct header), 1, f) > 0, "Failed to write header.\n");

    size_t offset = sizeof(struct header) + (sizeof(struct levelhdr) * header.levelcount);

    struct work work = {
        .header = &header,
        .offset = &offset,
        .f = f
    };

    // Callback iterate over all levels and faces to allow manipulation of output at the end, only at the first bit of data for a level will we actually write out a header.
    ktxTexture_IterateLevels(texture, (PFNKTXITERCB)iteratektx2, &work);

    ktxTexture_Destroy(texture);
    fclose(f);
}


int main(int argc, const char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s input output\n", argv[0]);
        return 1;
    }

    if (endswith(argv[1], ".ktx2")) {
        convertktx2(argv[1], argv[2]);
    } else {
        fprintf(stderr, "Invalid input file.\n");
    }


    return 0;
}
