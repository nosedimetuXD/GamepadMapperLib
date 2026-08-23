// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>

namespace GamepadMapper {

enum class Button : uint32_t {
    A = 0,
    B,
    X,
    Y,
    LStick,
    RStick,
    L,
    R,
    ZL,
    ZR,
    Plus,
    Minus,
    DLeft,
    DUp,
    DRight,
    DDown,
    SLLeft,
    SRLeft,
    Home,
    Screenshot,
    SLRight,
    SRRight,
    Count
};

enum class Stick : uint32_t {
    Left = 0,
    Right,
    Count
};

enum class Trigger : uint32_t {
    Left = 0,
    Right,
    Count
};

enum class Motion : uint32_t {
    Left = 0,
    Right,
    Count
};

enum class ControllerType : uint32_t {
    ProController = 0,
    DualJoycon,
    LeftJoycon,
    RightJoycon,
    Handheld,
    GameCube
};

struct StickState {
    float x = 0.0f; // -1.0f (left) to 1.0f (right)
    float y = 0.0f; // -1.0f (down) to 1.0f (up)
};

struct TriggerState {
    float value = 0.0f; // 0.0f (released) to 1.0f (fully pressed)
    bool pressed = false;
};

struct MotionState {
    float accel_x = 0.0f;
    float accel_y = 0.0f;
    float accel_z = 0.0f;
    float gyro_x = 0.0f;
    float gyro_y = 0.0f;
    float gyro_z = 0.0f;
    float quat_w = 1.0f;
    float quat_x = 0.0f;
    float quat_y = 0.0f;
    float quat_z = 0.0f;
};

struct GamepadState {
    bool is_connected = false;
    ControllerType type = ControllerType::ProController;
    uint32_t buttons = 0; // Bitmask of Button enum
    StickState left_stick{};
    StickState right_stick{};
    TriggerState left_trigger{};
    TriggerState right_trigger{};
    MotionState motion{};
    uint8_t battery_percentage = 100;
    bool is_charging = false;
};

} // namespace GamepadMapper
