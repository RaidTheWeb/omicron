#ifndef _SCRIPTING__SCRIPT_HPP
#define _SCRIPTING__SCRIPT_HPP

#include <assertion.hpp>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum {
    SCRIPT_TYPEGENERIC,
    SCRIPT_TYPEBEHAVIOUR,
    SCRIPT_TYPEPIPELINE
};

struct game_manager;
struct game_object;

struct pipeline;
struct renderer;

struct script {
    void *dl; // dlopen pointer
    int (*init)(void *); // generic script initialise

    size_t type;
    const char *id; // script name (identifier)
    const char *path; // path to script
    union {
        struct {
            void (*ready)(struct game_object *obj);
            void (*update)(struct game_object *obj);
            void (*render)(struct game_object *obj);
            void (*aiupdate)(struct game_object *obj);
            void (*signal)(struct game_object *obj, size_t signal, void *data);
        } behaviour;
        struct {
            void (*render)(struct pipeline *pipeline, struct renderer *renderer, size_t flags);
            void (*setup)(struct pipeline *pipeline);
            void *resources;
        } pipeline;
    };
};

struct script *script_load(const char *filename, const char *id);
void script_close(struct script *script);
void script_reload(struct script **script);

#endif
