#include <scripting/script.hpp>


struct script *script_load(const char *filename, const char *id) {
    struct script *script = (struct script *)malloc(sizeof(struct script));
    script->dl = dlopen(filename, RTLD_NOW);
    if (script->dl == NULL) {
        ASSERT(false, "Failed to load script %s with error '%s'\n", filename, dlerror());
        return NULL;
    }

    void *type = dlsym(script->dl, "script_type"); // find script type
    if (type == NULL) {
        ASSERT(false, "Failed to find `script_type` global variable in script %s\n", filename);
        return NULL;
    } else {
        script->type = *((size_t *)type);
    }

    script->init = (int (*)(void *))dlsym(script->dl, "script_init"); // find init function
    if (script->init == NULL) {
        ASSERT(false, "Failed to find `script_init` function in script %s\n", filename);
        return NULL;
    }

    switch (script->type) {
        case SCRIPT_TYPEBEHAVIOUR: {
            script->behaviour.ready = (void (*)(struct game_object *))dlsym(script->dl, "behaviour_ready");
            script->behaviour.update = (void (*)(struct game_object *))dlsym(script->dl, "behaviour_update");
            script->behaviour.render = (void (*)(struct game_object *))dlsym(script->dl, "behaviour_render");
            script->behaviour.signal = (void (*)(struct game_object *, size_t, void *))dlsym(script->dl, "behaviour_signal");
            script->behaviour.aiupdate = (void (*)(struct game_object *))dlsym(script->dl, "behaviour_aiupdate");
            break;
        }
        case SCRIPT_TYPEPIPELINE: {
            script->pipeline.resources = dlsym(script->dl, "pipeline_resources");
            script->pipeline.render = (void (*)(struct pipeline *, struct renderer *, size_t))dlsym(script->dl, "pipeline_render");
            script->pipeline.setup = (void (*)(struct pipeline *))dlsym(script->dl, "pipeline_setup");
            // script->pipeline.stages = dlsym(script->dl, "pipeline_stages");
            // script->pipeline.views = dlsym(script->dl, "pipeline_views");
            // void *stagecount = dlsym(script->dl, "pipeline_stagecount");
            // if (stagecount != NULL) {
            //     script->pipeline.stagecount = *((size_t *)stagecount);
            // } else {
            //     script->pipeline.stagecount = 0;
            // }
            // void *viewcount = dlsym(script->dl, "pipeline_viewcount");
            // if (viewcount != NULL) {
            //     script->pipeline.viewcount = *((size_t *)viewcount);
            // } else {
            //     script->pipeline.viewcount = 0;
            // }
            break;
        }
    }


    script->id = id;
    script->path = filename;

    return script;
}

void script_close(struct script *script) {
    dlclose(script->dl);
    free(script);
}

void dyn_reload(struct script **script) {
    char path[256];
    strncpy(path, (*script)->path, sizeof(path));
    char id[64];
    strncpy(id, (*script)->id, sizeof(id));
    script_close(*script);
    *script = script_load(path, id);
}
