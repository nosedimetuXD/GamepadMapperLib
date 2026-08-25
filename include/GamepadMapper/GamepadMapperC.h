// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
  #ifdef GAMEPADMAPPER_EXPORTS
    #define GAMEPAD_API __declspec(dllexport)
  #else
    #define GAMEPAD_API __declspec(dllimport)
  #endif
#else
  #define GAMEPAD_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAMEPAD_BUTTON_A = 0,
    GAMEPAD_BUTTON_B,
    GAMEPAD_BUTTON_X,
    GAMEPAD_BUTTON_Y,
    GAMEPAD_BUTTON_LSTICK,
    GAMEPAD_BUTTON_RSTICK,
    GAMEPAD_BUTTON_L,
    GAMEPAD_BUTTON_R,
    GAMEPAD_BUTTON_ZL,
    GAMEPAD_BUTTON_ZR,
    GAMEPAD_BUTTON_PLUS,
    GAMEPAD_BUTTON_MINUS,
    GAMEPAD_BUTTON_DLEFT,
    GAMEPAD_BUTTON_DUP,
    GAMEPAD_BUTTON_DRIGHT,
    GAMEPAD_BUTTON_DDOWN,
    GAMEPAD_BUTTON_SL_LEFT,
    GAMEPAD_BUTTON_SR_LEFT,
    GAMEPAD_BUTTON_HOME,
    GAMEPAD_BUTTON_SCREENSHOT,
    GAMEPAD_BUTTON_SL_RIGHT,
    GAMEPAD_BUTTON_SR_RIGHT,
    GAMEPAD_BUTTON_COUNT
} GamepadButton;

typedef enum {
    GAMEPAD_STICK_LEFT = 0,
    GAMEPAD_STICK_RIGHT,
    GAMEPAD_STICK_COUNT
} GamepadStick;

typedef enum {
    GAMEPAD_TRIGGER_LEFT = 0,
    GAMEPAD_TRIGGER_RIGHT,
    GAMEPAD_TRIGGER_COUNT
} GamepadTrigger;

typedef enum {
    GAMEPAD_CONTROLLER_PRO = 0,
    GAMEPAD_CONTROLLER_DUAL_JOYCON,
    GAMEPAD_CONTROLLER_LEFT_JOYCON,
    GAMEPAD_CONTROLLER_RIGHT_JOYCON,
    GAMEPAD_CONTROLLER_HANDHELD,
    GAMEPAD_CONTROLLER_GAMECUBE
} GamepadControllerType;

#pragma pack(push, 1)
typedef struct {
    float x; // -1.0f (left) to 1.0f (right)
    float y; // -1.0f (down) to 1.0f (up)
} GamepadStickState;

typedef struct {
    float value;   // 0.0f to 1.0f
    bool pressed;
} GamepadTriggerState;

typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float quat_w;
    float quat_x;
    float quat_y;
    float quat_z;
} GamepadMotionState;

typedef struct {
    bool is_connected;
    uint32_t type;
    uint32_t buttons; // Bitmask of GamepadButton
    GamepadStickState left_stick;
    GamepadStickState right_stick;
    GamepadTriggerState left_trigger;
    GamepadTriggerState right_trigger;
    GamepadMotionState motion;
    uint8_t battery_percentage;
    bool is_charging;
} GamepadFullState;
#pragma pack(pop)

// Lifecycle
GAMEPAD_API bool gamepad_initialize(void);
GAMEPAD_API void gamepad_shutdown(void);
GAMEPAD_API void gamepad_update(void);
GAMEPAD_API void gamepad_reload(void);

// Polling
GAMEPAD_API bool gamepad_is_connected(int player);
GAMEPAD_API int gamepad_get_type(int player);
GAMEPAD_API bool gamepad_is_button_pressed(int player, int button);
GAMEPAD_API bool gamepad_get_stick(int player, int stick, float* out_x, float* out_y);
GAMEPAD_API bool gamepad_get_trigger(int player, int trigger, float* out_value, bool* out_pressed);
GAMEPAD_API bool gamepad_get_motion(int player, GamepadMotionState* out_motion);
GAMEPAD_API bool gamepad_get_state(int player, GamepadFullState* out_state);

// Rumble / Feedback
GAMEPAD_API void gamepad_set_vibration(int player, float low_frequency, float high_frequency);

// Profile management
GAMEPAD_API bool gamepad_save_profile(int player, const char* profile_name);
GAMEPAD_API bool gamepad_load_profile(int player, const char* profile_name);

// GUI Configuration
GAMEPAD_API bool gamepad_show_config_dialog(void* parent_window);
GAMEPAD_API bool gamepad_show_single_config_dialog(void* parent_window);

#ifdef __cplusplus
}
#endif
