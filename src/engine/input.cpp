#include <engine/engine.hpp>
#include <engine/event.hpp>
#include <engine/game.hpp>
#include <engine/input.hpp>

struct input_manager input_manager;

static void input_joystickevent(uint8_t jid, int event) {
    if (event == GLFW_CONNECTED && glfwJoystickIsGamepad(jid)) {
        input_manager.controllers[jid].name = glfwGetGamepadName(jid);
        input_manager.controllers[jid].id = jid; // initially set up our id (just for referencing ourselves based on the id we have)
        printf("Controller device '%s' connected on %u\n", input_manager.controllers[jid].name, jid);
        input_manager.controllers[jid].state.connected = true; // connection will be only accepted on a working "gamepad"
        struct event event = { };
        event.type = utils_stringid("omicron_controllerconnect");
        event.numargs = 1;
        event.args[0] = jid;
        event.deliverytime = utils_getcounter(); // immediate dispatch
        event_queue(event);
    } else if (event == GLFW_DISCONNECTED && input_manager.controllers[jid].state.connected) { // we can assume that it's actually a gamepad because it'd only ever be marked connected if it was in the first place.
        printf("Controller device '%s' disconnected from %u\n", input_manager.controllers[jid].name, jid);
        input_manager.controllers[jid].name = "Unknown";
        input_manager.controllers[jid].state.connected = false;
        struct event event = { };
        event.type = utils_stringid("omicron_controllerdisconnect");
        event.numargs = 1;
        event.args[0] = jid;
        event.deliverytime = utils_getcounter(); // immediate dispatch
        event_queue(event);
    }
}

static void input_keyboard(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;
    (void)mods;
    input_manager.keys[key] = action;
}

void input_init(void) {
    for (size_t i = 0; i < INPUT_NUMCONTROLLERS; i++) {
        if (glfwJoystickIsGamepad(i)) {
            input_manager.controllers[i].name = glfwGetGamepadName(i);
            input_manager.controllers[i].id = i; // initially set up our id (just for referencing ourselves based on the id we have)
            printf("Controller device '%s' connected on %lu\n", input_manager.controllers[i].name, i);
            input_manager.controllers[i].state.connected = true; // connection will be only accepted on a working "gamepad"
            struct event event = { };
            event.type = utils_stringid("omicron_controllerconnect");
            event.numargs = 1;
            event.args[0] = i;
            event.deliverytime = utils_getcounter(); // immediate dispatch
            event_queue(event);
        }
        input_manager.controllers[i].sensitivity = glm::vec2(1.25f, 1.25f); // TODO: this should be read from a configuration file
        input_manager.controllers[i].deadzone = 0.41f; // TODO: ditto
    }

    glfwSetJoystickCallback((GLFWjoystickfun)input_joystickevent);
    glfwSetInputMode(engine_engine->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(engine_engine->window, input_keyboard);

    input_manager.mousesensitivity = glm::vec2(1.0f, 1.0f); // TODO: this should be read from a configuration file
}

static bool input_poll = false;

void input_resetpoll(void) {
    input_poll = false;
}

static void input_querypoll(void) {
    if (!input_poll) {
        glfwPollEvents();
        input_poll = true;
    }
}

void input_updateframe(void) {
    input_querypoll();

    double mx, my;
    glfwGetCursorPos(engine_engine->window, &mx, &my);
    input_manager.mousepos = glm::vec2(mx, my);
}

static void input_controllerstate(uint8_t id) {
    GLFWgamepadstate state;
    glfwGetGamepadState(id, &state);

    struct input_controllerstate *ics = &input_manager.controllers[id].state;
    for (size_t i = 0; i < sizeof(ics->buttons); i++) {
        ics->buttons[i] = state.buttons[i];
    }

    for (size_t i = 0; i < sizeof(ics->axes) / sizeof(ics->axes[0]); i++) {
        ics->axes[i] = state.axes[i];
    }
}

void input_update(void) {
    input_querypoll();
    for (size_t i = 0; i < INPUT_MOUSECOUNT; i++) {
        input_manager.mousebuttons[i] = glfwGetMouseButton(engine_engine->window, i);
    }

    for (size_t i = 0; i < INPUT_NUMCONTROLLERS; i++) {
        if (input_manager.controllers[i].state.connected) {
            input_controllerstate(i);
        }
    }
}

bool input_controllerbutton(uint8_t id, uint8_t button) {
    struct input_controller *controller = &input_manager.controllers[id];

    ASSERT(button < INPUT_GAMEPADBUTTONCOUNT, "Invalid button %u.\n", button);

    if (!controller->state.connected) {
        return false; // obviously, it's not pressed if it's not connected
    }

    return controller->state.buttons[button];
}

float input_controlleraxis(uint8_t id, uint8_t axis) {
    struct input_controller *controller = &input_manager.controllers[id];

    ASSERT(axis < INPUT_GAMEPADAXISCOUNT, "Invalid axis %u.\n", axis);

    if (!controller->state.connected) {
        return 0.0f;
    }

    return controller->state.axes[axis] > controller->deadzone ? controller->state.axes[axis] : controller->state.axes[axis] < -controller->deadzone ? controller->state.axes[axis] : 0.0f;
}
