// SPDX-License-Identifier: MIT
#include "GamepadMapper/GamepadManager.h"
#include "GamepadMapper/GamepadConfigDialog.h"

#include <chrono>
#include <thread>
#include <QApplication>

#include "common/settings.h"
#include "core/hid_core/hid_core.h"
#include "core/hid_core/frontend/emulated_controller.h"
#include "core/input_common/main.h"
#include "ui/input_profiles.h"
#include "ui/qt_config.h"

namespace GamepadMapper {

struct GamepadManager::Impl {
    std::unique_ptr<InputCommon::InputSubsystem> input_subsystem;
    std::unique_ptr<Core::HID::HIDCore> hid_core;
    std::unique_ptr<InputProfiles> profiles;
    std::unique_ptr<QtConfig> qt_config;
    bool initialized{false};
};

GamepadManager& GamepadManager::Instance() {
    static GamepadManager instance;
    return instance;
}

GamepadManager::GamepadManager() : impl{std::make_unique<Impl>()} {}

GamepadManager::~GamepadManager() {
    Shutdown();
}

bool GamepadManager::Initialize() {
    if (impl->initialized) {
        return true;
    }

    impl->input_subsystem = std::make_unique<InputCommon::InputSubsystem>();
    impl->input_subsystem->Initialize();

    impl->hid_core = std::make_unique<Core::HID::HIDCore>();
    impl->profiles = std::make_unique<InputProfiles>();
    impl->qt_config = std::make_unique<QtConfig>();
    impl->qt_config->ReloadAllValues();

    impl->hid_core->ReloadInputDevices();

    impl->initialized = true;
    return true;
}

void GamepadManager::Shutdown() {
    if (!impl->initialized) {
        return;
    }

    if (impl->qt_config) {
        impl->qt_config->SaveAllValues();
    }

    impl->profiles.reset();
    impl->qt_config.reset();
    impl->hid_core.reset();

    if (impl->input_subsystem) {
        impl->input_subsystem->Shutdown();
        impl->input_subsystem.reset();
    }

    impl->initialized = false;
}

void GamepadManager::Update() {
    if (!impl->initialized) {
        return;
    }

    impl->input_subsystem->PumpEvents();
}

bool GamepadManager::IsConnected(int player) const {
    if (!impl->initialized || player < 0 || player >= 8) {
        return false;
    }
    auto controller = impl->hid_core->GetEmulatedControllerByIndex(player);
    return controller && controller->IsConnected();
}

ControllerType GamepadManager::GetType(int player) const {
    if (!impl->initialized || player < 0 || player >= 8) {
        return ControllerType::ProController;
    }
    auto controller = impl->hid_core->GetEmulatedControllerByIndex(player);
    if (!controller) {
        return ControllerType::ProController;
    }
    return static_cast<ControllerType>(controller->GetNpadStyleIndex());
}

bool GamepadManager::IsButtonPressed(int player, Button button) const {
    if (!impl->initialized || player < 0 || player >= 8) {
        return false;
    }
    auto controller = impl->hid_core->GetEmulatedControllerByIndex(player);
    if (!controller || !controller->IsConnected()) {
        return false;
    }
    const auto npad = controller->GetNpadButtons();
    switch (button) {
    case Button::A: return npad.a != 0;
    case Button::B: return npad.b != 0;
    case Button::X: return npad.x != 0;
    case Button::Y: return npad.y != 0;
    case Button::LStick: return npad.stick_l != 0;
    case Button::RStick: return npad.stick_r != 0;
    case Button::L: return npad.l != 0;
    case Button::R: return npad.r != 0;
    case Button::ZL: return npad.zl != 0;
    case Button::ZR: return npad.zr != 0;
    case Button::Plus: return npad.plus != 0;
    case Button::Minus: return npad.minus != 0;
    case Button::DLeft: return npad.left != 0;
    case Button::DUp: return npad.up != 0;
    case Button::DRight: return npad.right != 0;
    case Button::DDown: return npad.down != 0;
    case Button::Home: return controller->GetHomeButtons().home != 0;
    case Button::Screenshot: return controller->GetCaptureButtons().capture != 0;
    default: return false;
    }
}

StickState GamepadManager::GetStick(int player, Stick stick) const {
    if (!impl->initialized || player < 0 || player >= 8) {
        return {};
    }
    auto controller = impl->hid_core->GetEmulatedControllerByIndex(player);
    if (!controller || !controller->IsConnected()) {
        return {};
    }
    const auto sticks = controller->GetSticks();
    const auto& s = (stick == Stick::Right) ? sticks.right : sticks.left;
    return StickState{
        .x = static_cast<float>(s.x) / 32767.0f,
        .y = static_cast<float>(s.y) / 32767.0f,
    };
}

TriggerState GamepadManager::GetTrigger(int player, Trigger trigger) const {
    if (!impl->initialized || player < 0 || player >= 8) {
        return {};
    }
    auto controller = impl->hid_core->GetEmulatedControllerByIndex(player);
    if (!controller || !controller->IsConnected()) {
        return {};
    }
    const auto triggers = controller->GetTriggers();
    const auto val = (trigger == Trigger::Right) ? triggers.right : triggers.left;
    const float norm_val = static_cast<float>(val) / 32767.0f;
    return TriggerState{
        .value = norm_val,
        .pressed = (norm_val > 0.5f),
    };
}

MotionState GamepadManager::GetMotion(int player) const {
    if (!impl->initialized || player < 0 || player >= 8) {
        return {};
    }
    auto controller = impl->hid_core->GetEmulatedControllerByIndex(player);
    if (!controller || !controller->IsConnected()) {
        return {};
    }
    const auto motions = controller->GetMotions();
    const auto& m = motions[0];
    return MotionState{
        .accel_x = m.accel.x,
        .accel_y = m.accel.y,
        .accel_z = m.accel.z,
        .gyro_x = m.gyro.x,
        .gyro_y = m.gyro.y,
        .gyro_z = m.gyro.z,
        .quat_w = 1.0f,
        .quat_x = m.rotation.x,
        .quat_y = m.rotation.y,
        .quat_z = m.rotation.z,
    };
}

GamepadState GamepadManager::GetState(int player) const {
    GamepadState state{};
    if (!impl->initialized || player < 0 || player >= 8) {
        return state;
    }
    state.is_connected = IsConnected(player);
    if (!state.is_connected) {
        return state;
    }

    state.type = GetType(player);
    uint32_t btn_mask = 0;
    for (uint32_t b = 0; b < static_cast<uint32_t>(Button::Count); ++b) {
        if (IsButtonPressed(player, static_cast<Button>(b))) {
            btn_mask |= (1u << b);
        }
    }
    state.buttons = btn_mask;
    state.left_stick = GetStick(player, Stick::Left);
    state.right_stick = GetStick(player, Stick::Right);
    state.left_trigger = GetTrigger(player, Trigger::Left);
    state.right_trigger = GetTrigger(player, Trigger::Right);
    state.motion = GetMotion(player);
    return state;
}

void GamepadManager::SetVibration(int player, float low_frequency, float high_frequency) {
    if (!impl->initialized || player < 0 || player >= 8) {
        return;
    }
    auto controller = impl->hid_core->GetEmulatedControllerByIndex(player);
    if (!controller) {
        return;
    }
    Core::HID::VibrationValue vib{
        .low_amplitude = low_frequency,
        .low_frequency = 160.0f,
        .high_amplitude = high_frequency,
        .high_frequency = 320.0f,
    };
    controller->SetVibration(Core::HID::DeviceIndex::Left, vib);
    controller->SetVibration(Core::HID::DeviceIndex::Right, vib);
}

bool GamepadManager::SaveProfile(int player, const std::string& profile_name) {
    if (!impl->initialized || !impl->profiles || player < 0 || player >= 8) {
        return false;
    }
    return impl->profiles->SaveProfile(profile_name, player);
}

bool GamepadManager::LoadProfile(int player, const std::string& profile_name) {
    if (!impl->initialized || !impl->profiles || player < 0 || player >= 8) {
        return false;
    }
    return impl->profiles->LoadProfile(profile_name, player);
}

std::vector<std::string> GamepadManager::GetProfileList() const {
    if (!impl->initialized || !impl->profiles) {
        return {};
    }
    return impl->profiles->GetInputProfileNames();
}

InputCommon::InputSubsystem* GamepadManager::GetInputSubsystem() {
    return impl->input_subsystem.get();
}

Core::HID::HIDCore* GamepadManager::GetHIDCore() {
    return impl->hid_core.get();
}

} // namespace GamepadMapper
