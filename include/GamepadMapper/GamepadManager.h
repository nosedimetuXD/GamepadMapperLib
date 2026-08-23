// SPDX-License-Identifier: MIT
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "GamepadTypes.h"

namespace Core::HID {
class HIDCore;
class EmulatedController;
}

namespace InputCommon {
class InputSubsystem;
}

namespace GamepadMapper {

class GamepadManager {
public:
    static GamepadManager& Instance();

    /// Initialize the input subsystem and controller devices
    bool Initialize();

    /// Shutdown all input drivers and release resources
    void Shutdown();

    /// Update controller states (call once per frame)
    void Update();

    /// Check if player (0..7) is connected
    [[nodiscard]] bool IsConnected(int player) const;

    /// Get controller type for player (0..7)
    [[nodiscard]] ControllerType GetType(int player) const;

    /// Query if button is pressed for player (0..7)
    [[nodiscard]] bool IsButtonPressed(int player, Button button) const;

    /// Get stick position for player (0..7)
    [[nodiscard]] StickState GetStick(int player, Stick stick) const;

    /// Get trigger state for player (0..7)
    [[nodiscard]] TriggerState GetTrigger(int player, Trigger trigger) const;

    /// Get motion state for player (0..7)
    [[nodiscard]] MotionState GetMotion(int player) const;

    /// Get full gamepad snapshot for player (0..7)
    [[nodiscard]] GamepadState GetState(int player) const;

    /// Set vibration / rumble (low_frequency and high_frequency 0.0f..1.0f)
    void SetVibration(int player, float low_frequency, float high_frequency);

    /// Profile management
    bool SaveProfile(int player, const std::string& profile_name);
    bool LoadProfile(int player, const std::string& profile_name);
    std::vector<std::string> GetProfileList() const;

    /// Direct access to internal subsystems if needed
    InputCommon::InputSubsystem* GetInputSubsystem();
    Core::HID::HIDCore* GetHIDCore();

private:
    GamepadManager();
    ~GamepadManager();

    GamepadManager(const GamepadManager&) = delete;
    GamepadManager& operator=(const GamepadManager&) = delete;

    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace GamepadMapper
