#ifndef _ENGINE__GAME_HPP
#define _ENGINE__GAME_HPP

#include <engine/input.hpp>
#include <engine/model.hpp>
#include <resources/resource.hpp>
#include <scripting/script.hpp>

// game object generic type
struct game_object {
    glm::mat4 transform; // transform matrix
    struct model *model; // model (multiple meshes bound with information dictating material information)
    void *userdata; // userdata; specific to game objects (contains individual information for the object)
    struct script *script; // script that defines our game object variables

    void (*ready)(struct game_object *obj); // object ready (called when object is initialised)
    void (*update)(struct game_object *obj); // object physics update (physics update, called at a fixed rate)
    void (*render)(struct game_object *obj); // object render update (called every frame)
    void (*aiupdate)(struct game_object *obj); // object update ai (called at large periodic fixed rate, ai updates don't need to happen every physics tick)
    void (*signal)(struct game_object *obj, size_t signal, void *data); // signal a flag to a game object with some data (blocking call, jump between object context)
};

struct game_object *game_initobj(void);
void game_objbindscript(struct game_object *obj, struct script *script);

#endif
