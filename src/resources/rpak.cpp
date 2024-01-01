#include <dirent.h>
#include <resources/rpak.hpp>
#include <zlib.h>

namespace OResource {

    // struct rpak_file *rpak_mount(const char *path) {
    RPak::RPak(const char *path) {
        ASSERT(path, "Mount NULL path.\n");

        FILE *f = fopen(path, "r");
        ASSERT(f, "Failed to open RPAK to mount.\n");
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);

        struct RPak::header header = { 0 };
        ASSERT(fread(&header, sizeof(struct RPak::header), 1, f), "Failed to read RPAK table entries.\n");
        ASSERT(!strncmp(header.magic, "RPAK", sizeof(header.magic)), "Attempting to mount RPAK file that does not display magic.\n");

        struct RPak::tableentry *entries = (struct RPak::tableentry *)malloc(sizeof(struct RPak::tableentry) * header.num);
        fread(entries, sizeof(struct RPak::tableentry) * header.num, 1, f);

        // struct rpak_file *rpak = (struct rpak_file *)malloc(sizeof(struct rpak_file));
        this->entries = entries;
        this->header = header;
        this->file = f;
    }

    // struct rpak_stat rpak_stat(struct rpak_file *file, const char *path) {
    struct RPak::stat RPak::stat(const char *path) {
    JOB_MUTEXSAFE(&this->lock,
        ASSERT(path, "Attempting to read NULL file path.\n");

        struct RPak::tableentry *entry = NULL;
        for (size_t i = 0; i < this->header.num; i++) {
            if (!strncmp(this->entries[i].path, path, 512)) {
                entry = &this->entries[i];
                goto found;
            }
        }


        JOB_MUTEXSAFEEARLY(&this->lock);
        return (struct RPak::stat) { .realsize = SIZE_MAX, .decompressedsize = SIZE_MAX, .compressed = true };
    found:
    );
        return (struct RPak::stat) { .realsize = entry->compressed ? entry->compressedsize : entry->uncompressedsize, .decompressedsize = entry->uncompressedsize, .compressed = entry->compressed };
    }

    // size_t rpak_read(struct rpak_file *file, const char *path, void *buf, size_t size, const size_t off) {
    size_t RPak::read(const char *path, void *buf, const size_t size, const size_t off) {
    JOB_MUTEXSAFE(&this->lock,
        ASSERT(path, "Attempting to read NULL file path.\n");
        ASSERT(buf, "Attempting to read into NULL buffer.\n");
        ASSERT(size > 0, "Read zero bytes.\n");

        struct RPak::tableentry *entry = NULL;
        for (size_t i = 0; i < this->header.num; i++) {
            if (!strncmp(this->entries[i].path, path, 512)) {
                entry = &this->entries[i];
                goto found;
            }
        }

        JOB_MUTEXSAFEEARLY(&this->lock);
        return 0;
    found:
        size_t finalsize = size;
        if (off + size > entry->uncompressedsize) {
            finalsize = size - ((off + size) - entry->uncompressedsize);
        }
        fseek(this->file, entry->offset + off, SEEK_SET);
        if (!entry->compressed) {
            ASSERT(fread(buf, size, 1, this->file), "Failed to read file from RPAK.\n");
        } else {
            uint8_t *tbuf = (uint8_t *)malloc(entry->uncompressedsize + RPAK_COMPRESSIONFAULTTOLERANCE);
            uint8_t *from = (uint8_t *)malloc(entry->compressedsize);
            fseek(this->file, entry->offset, SEEK_SET);
            fread(from, entry->compressedsize, 1, this->file);
            size_t decompressed = entry->uncompressedsize + RPAK_COMPRESSIONFAULTTOLERANCE;
            int res = uncompress(tbuf, &decompressed, from, entry->compressedsize);
            free(from);
            ASSERT(res == Z_OK, "Failed to decompress RPAK file data.\n");
            memcpy(buf, tbuf + off, size);
            free(tbuf);
        }
    );
        return size;
    }

    static size_t listdir(int counter, char *path, char *gdpath, struct RPak::tableentry **tables, size_t *tableidx, struct RPak::data **data, size_t *dataidx, size_t *datasize) {
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

                struct RPak::tableentry entry = { 0 };
                FILE *f = fopen(fpath, "r");
                ASSERT(f, "Failed to open file for RPAK compression.\n");
                fseek(f, 0, SEEK_END);
                entry.uncompressedsize = ftell(f);
                fseek(f, 0, SEEK_SET);
                entry.offset = 0;
                strncpy(entry.path, dpath, 512);
                entry.compressed = (entry.uncompressedsize > RPAK_COMPRESSBIAS);

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
                *tables = (struct RPak::tableentry *)realloc(*tables, sizeof(struct RPak::tableentry) * ((*tableidx) + 1));
                entry.offset = outsize;
                struct RPak::data *d = *data;
                d[*dataidx] = (struct RPak::data) { .data = (uint8_t *)malloc(outsize), .size = outsize };
                memcpy(d[*dataidx].data, entry.compressed ? compressed : fdata, outsize);
                (*dataidx)++;
                *datasize += outsize;
                *data = (struct RPak::data *)realloc(*data, sizeof(struct RPak::data) * ((*dataidx) + 1));

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

    void RPak::create(const char *path, const char *output) {
        ASSERT(strlen(path) < 512, "Path too long.\n");
        ASSERT(strlen(output) < 64, "Output path too long.\n");

        struct RPak::header header = { 0 };
        strcpy(header.magic, "RPAK");
        header.version = RPAK_FORMATVERSION;
        header.contentversion = 1;
        strncpy(header.name, output, 64);

        size_t tableidx = 0;
        size_t dataidx = 0;
        size_t datasize = 0;
        struct RPak::tableentry *tables = (struct RPak::tableentry *)malloc(sizeof(struct RPak::tableentry));
        struct RPak::data *data = (struct RPak::data *)malloc(sizeof(struct RPak::data));
        size_t count = listdir(1, (char *)path, (char *)path, &tables, &tableidx, &data, &dataidx, &datasize);

        header.num = count;

        size_t outputsize = sizeof(struct RPak::header) + (sizeof(struct RPak::tableentry) * count) + (datasize);
        uint8_t *rpakdata = (uint8_t *)malloc(outputsize);
        memcpy(rpakdata, &header, sizeof(struct RPak::header));
        size_t off = sizeof(struct RPak::header);
        size_t dataoff = sizeof(struct RPak::header);
        for (size_t i = 0; i < tableidx; i++) {
            struct RPak::tableentry entry = tables[i];
            entry.offset += dataoff + (sizeof(struct RPak::tableentry) * (tableidx)); // update offset
            dataoff += data[i].size;
            memcpy(rpakdata + off, &entry, sizeof(struct RPak::tableentry));
            off += (sizeof(struct RPak::tableentry));
        }
        free(tables);

        for (size_t i = 0; i < dataidx; i++) {
            struct RPak::data rdata = data[i];
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

}
