#ifndef _ENGINE__INPUT_HPP
#define _ENGINE__INPUT_HPP

#include <GLFW/glfw3.h>
#include <engine/math/math.hpp>
#include <stdbool.h>
#include <stdint.h>

#define INPUT_GAMEPADBUTTONA 0
#define INPUT_GAMEPADBUTTONB 1
#define INPUT_GAMEPADBUTTONX 2
#define INPUT_GAMEPADBUTTONY 3
#define INPUT_GAMEPADBUTTONLB 4
#define INPUT_GAMEPADBUTTONRB 5
#define INPUT_GAMEPADBUTTONBACK 6
#define INPUT_GAMEPADBUTTONSTART 7
#define INPUT_GAMEPADBUTTONGUIDE 8
#define INPUT_GAMEPADBUTTONLS 9
#define INPUT_GAMEPADBUTTONRS 10
#define INPUT_GAMEPADBUTTONDPADUP 11
#define INPUT_GAMEPADBUTTONDPADRIGHT 12
#define INPUT_GAMEPADBUTTONDPADDOWN 13
#define INPUT_GAMEPADBUTTONDPADLEFT 14
#define INPUT_GAMEPADBUTTONCOUNT 15

#define INPUT_GAMEPADBUTTONCROSS INPUT_GAMEPADBUTTONA
#define INPUT_GAMEPADBUTTONCIRCLE INPUT_GAMEPADBUTTONB
#define INPUT_GAMEPADBUTTONSQUARE INPUT_GAMEPADBUTTONX
#define INPUT_GAMEPADBUTTONTRIANGLE INPUT_GAMEPADBUTTONY
#define INPUT_GAMEPADBUTTONSELECT INPUT_GAMEPADBUTTONBACK
#define INPUT_GAMEPADBUTTONOPTIONS INPUT_GAMEPADBUTTONBACK
#define INPUT_GAMEPADBUTTONPS INPUT_GAMEPADBUTTONGUIDE

#define INPUT_GAMEPADAXISLEFTX 0
#define INPUT_GAMEPADAXISLEFTY 1
#define INPUT_GAMEPADAXISRIGHTX 2
#define INPUT_GAMEPADAXISRIGHTY 3
#define INPUT_GAMEPADAXISLT 4
#define INPUT_GAMEPADAXISRT 5
#define INPUT_GAMEPADAXISCOUNT 6

struct input_controllerstate {
    uint8_t buttons[15];
    float axes[6];
    bool connected;
};

#define INPUT_NUMCONTROLLERS 4

struct input_controller {
    struct input_controllerstate state;
    const char *name;
    uint8_t id;
    glm::vec2 sensitivity;
    float deadzone;
};

enum {
    INPUT_MOUSELEFT,
    INPUT_MOUSEMIDDLE,
    INPUT_MOUSERIGHT,
    INPUT_MOUSE4,
    INPUT_MOUSE5,
    INPUT_MOUSE6,
    INPUT_MOUSE7,
    INPUT_MOUSE8,
    INPUT_MOUSECOUNT
};

#define INPUT_NUMKEYS 348

struct game_manager;

struct input_manager {
    struct game_manager *game;
    struct input_controller controllers[INPUT_NUMCONTROLLERS];
    bool keys[INPUT_NUMKEYS];
    bool mousebuttons[INPUT_MOUSECOUNT];
    glm::vec2 mousepos;
    glm::vec2 lastmousepos;
    int backend;
    glm::vec2 mousesensitivity;
};

extern struct input_manager input_manager;

void input_init(void);
void input_update(void);
void input_updateframe(void);
void input_resetpoll(void);

bool input_controllerbutton(uint8_t id, uint8_t button);
float input_controlleraxis(uint8_t id, uint8_t axis);

#endif
