#include <engine/event.hpp>
#include <dlfcn.h>
#include <engine/game.hpp>
#include <concurrency/job.hpp>

static void game_objdefault() { // polymorphic
    ; // literally does nothing with the arguments or otherwise
}

struct game_object *game_initobj(void) {
    struct game_object *obj = (struct game_object *)malloc(sizeof(struct game_object));
    obj->transform = glm::mat4(1.0f);
    obj->render = (void (*)(struct game_object *))game_objdefault;
    obj->update = (void (*)(struct game_object *))game_objdefault;
    obj->signal = (void (*)(struct game_object *, size_t, void *))game_objdefault;
    obj->aiupdate = (void (*)(struct game_object *))game_objdefault;
    obj->model = NULL;
    obj->userdata = NULL;
    obj->script = NULL;
    return obj;
}

void game_objbindscript(struct game_object *obj, struct script *script) {
    ASSERT(script->type == SCRIPT_TYPEBEHAVIOUR, "Script is not of type game object!\n");
    obj->ready = script->behaviour.ready ? script->behaviour.ready : (void (*)(struct game_object *))game_objdefault;
    obj->update = script->behaviour.update ? script->behaviour.update : (void (*)(struct game_object *))game_objdefault;
    obj->render = script->behaviour.render ? script->behaviour.render : (void (*)(struct game_object *))game_objdefault;
    obj->signal = script->behaviour.signal ? script->behaviour.signal : (void (*)(struct game_object *, size_t, void *))game_objdefault;
    obj->aiupdate = script->behaviour.aiupdate ? script->behaviour.aiupdate : (void (*)(struct game_object *))game_objdefault;
}


// void testeventlistener(void *_, struct event *event) {
//     (void)_;
//
//     printf("event triggered! argument 0: %s, argument 1: %s\n", event->args[0], event->args[1]);
// }
//
// void testprocedure(struct job_jobdecl *job) {
//     printf("listen event.\n");
//     event_listen(event_type("test"), testeventlistener, NULL);
//
//     struct event event = { 0 };
//     event.type = event_type("test");
//     event.numargs = 2;
//     event.args[0] = (uintptr_t)"hi";
//     event.args[1] = (uintptr_t)"bye";
//     event.deliverytime = 200;
//     event_queue(event);
//
//     event_dispatchevents(400);
// }

// void game_init(bgfx_renderer_type_t suggest) {
//     job_init();
//     event_init();
//     resource_init();
//
//     game_manager.renderers = resource_ringinit(16);
//     game_manager.globalrenderer = render_rendererringpush(game_manager.renderers, (struct render_renderer) { 0 });
//     game_manager.globalrenderer->game = &game_manager;
//     // renderer_init(game_manager.globalrenderer, GAME_BACKEND_GLFW, suggest); // TODO: Multi-backend support (if at all needed)
//
//     game_manager.inputmanager.backend = GAME_BACKEND_GLFW;
//     game_manager.inputmanager.game = &game_manager;
//     input_managerinit(&game_manager.inputmanager);
// }
