#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define ASSERT(cond, ...) ({ \
    if (!(cond)) { \
        printf("Assertion (" #cond ") failed in %s, function %s on line %d:\n\t", __FILE__, __FUNCTION__, __LINE__); \
        printf(__VA_ARGS__); \
        abort(); \
    } \
})

#define RPAK_FORMATVERSION 1
#define RPAK_COMPRESSBIAS (16 * 1024 * 1024) // above 16MB we compress the file
#define RPAK_COMPRESSIONFAULTTOLERANCE (32) // fault tolerance for compression buffers

struct header {
    char magic[5]; // RPAK\0
    uint8_t version; // version of RPak
    uint32_t contentversion; // version of this particular RPak archive
    char name[64];
    uint32_t num; // number of package entries
} __attribute__((packed));

struct tableentry {
    char path[512]; // full pathname for the file in RPak
    bool compressed; // is this file compressed?
    size_t uncompressedsize; // original file size
    size_t compressedsize; // file size when compressed
    size_t offset; // offset of file data in blob
} __attribute__((packed));

struct data {
    uint8_t *data;
    size_t size;
};

static size_t listdir(int counter, char *path, char *gdpath, struct tableentry **tables, size_t *tableidx, struct data **data, size_t *dataidx, size_t *datasize) {
    struct dirent *dir;
    DIR *d = opendir(path);
    if (d == NULL) {
        return 0;
    }
    size_t count = 0;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG) {
            count++;
            char dpath[512];
            char fpath[512];
            // if (counter == 1) {
                // snprintf(dpath, 512, "%s", dir->d_name);
            // } else {
                snprintf(dpath, 512, "%s/%s", gdpath, dir->d_name);
            // }
            snprintf(fpath, 512, "%s/%s", path, dir->d_name);

            struct tableentry entry = { 0 };
            FILE *f = fopen(fpath, "r");
            ASSERT(f, "Failed to open file for RPAK compression.\n");
            fseek(f, 0, SEEK_END);
            entry.uncompressedsize = ftell(f);
            fseek(f, 0, SEEK_SET);
            entry.offset = 0;
            strncpy(entry.path, dpath, 512);
            // XXX: Too expensive
            // entry.compressed = (entry.uncompressedsize > RPAK_COMPRESSBIAS);

            uint8_t *fdata = (uint8_t *)malloc(entry.uncompressedsize);
            ASSERT(fread(fdata, entry.uncompressedsize, 1, f), "Failed to read file for RPAK compression.\n");
            fclose(f);

            uint8_t *compressed = NULL;

            if (entry.compressed) {
                compressed = (uint8_t *)malloc(entry.uncompressedsize + RPAK_COMPRESSIONFAULTTOLERANCE); // more space for the compression (annoying if it gets too big)
                size_t compressedsize = entry.uncompressedsize + RPAK_COMPRESSIONFAULTTOLERANCE;
                int res = compress(compressed, &compressedsize, fdata, entry.uncompressedsize);
                ASSERT(res == Z_OK, "Compression failed.\n");
                entry.compressedsize = compressedsize;
                free(fdata);
            } else {
                entry.compressedsize = entry.uncompressedsize;
            }

            size_t outsize = entry.compressed ? entry.compressedsize : entry.uncompressedsize;
            (*tables)[*tableidx] = entry;
            (*tableidx)++;
            *tables = (struct tableentry *)realloc(*tables, sizeof(struct tableentry) * ((*tableidx) + 1));
            entry.offset = outsize;
            struct data *d = *data;
            d[*dataidx] = (struct data) { .data = (uint8_t *)malloc(outsize), .size = outsize };
            memcpy(d[*dataidx].data, entry.compressed ? compressed : fdata, outsize);
            (*dataidx)++;
            *datasize += outsize;
            *data = (struct data *)realloc(*data, sizeof(struct data) * ((*dataidx) + 1));

            if (entry.compressed) {
                free(compressed);
            } else {
                free(fdata);
            }

        } else if (dir->d_type == DT_DIR && strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
            char dpath[512];
            // if (counter == 1) {
                // snprintf(dpath, 256, "%s", dir->d_name);
            // } else {
                snprintf(dpath, 256, "%s/%s", gdpath, dir->d_name);
            // }
            char fpath[512];
            snprintf(fpath, 256, "%s/%s", path, dir->d_name);
            count += listdir(counter - 1, fpath, dpath, tables, tableidx, data, dataidx, datasize);
        }
    }
    closedir(d);

    return count;
}

void create(const char *path, const char *output) {
    ASSERT(strlen(path) < 512, "Path too long.\n");
    ASSERT(strlen(output) < 64, "Output path too long.\n");

    struct header header = { 0 };
    strcpy(header.magic, "RPAK");
    header.version = RPAK_FORMATVERSION;
    header.contentversion = 1;
    strncpy(header.name, output, 64);

    size_t tableidx = 0;
    size_t dataidx = 0;
    size_t datasize = 0;
    struct tableentry *tables = (struct tableentry *)malloc(sizeof(struct tableentry));
    struct data *data = (struct data *)malloc(sizeof(struct data));
    size_t count = listdir(1, (char *)path, (char *)path, &tables, &tableidx, &data, &dataidx, &datasize);

    header.num = count;

    size_t outputsize = sizeof(struct header) + (sizeof(struct tableentry) * count) + (datasize);
    uint8_t *rpakdata = (uint8_t *)malloc(outputsize);
    memcpy(rpakdata, &header, sizeof(struct header));
    size_t off = sizeof(struct header);
    size_t dataoff = sizeof(struct header);
    for (size_t i = 0; i < tableidx; i++) {
        struct tableentry entry = tables[i];
        entry.offset += dataoff + (sizeof(struct tableentry) * (tableidx)); // update offset
        dataoff += data[i].size;
        memcpy(rpakdata + off, &entry, sizeof(struct tableentry));
        off += (sizeof(struct tableentry));
    }
    free(tables);

    for (size_t i = 0; i < dataidx; i++) {
        struct data rdata = data[i];
        memcpy(rpakdata + off, data[i].data, data[i].size);
        off += data[i].size;
        free(data[i].data);
    }
    free(data);

    FILE *out = fopen(output, "w");
    ASSERT(out, "Failed to open output RPAK.\n");
    ASSERT(fwrite(rpakdata, outputsize, 1, out), "Failed to write RPAK.\n");
    free(rpakdata);
    fclose(out);
}


int main(int argc, const char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s input output\n", argv[0]);
        return 1;
    }

    create(argv[1], argv[2]);

    return 0;
}
