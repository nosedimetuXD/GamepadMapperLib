// SPDX-License-Identifier: MIT
#pragma once

#include <QWidget>
#include <QTimer>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QGroupBox>
#include <array>
#include <functional>
#include <optional>
#include <memory>

#include "GamepadMapper/SingleGamepadConfigDialog.h"
#include "common/param_package.h"
#include "common/settings_input.h"
#include "core/hid_core/hid_types.h"
#include "core/input_common/main.h"

namespace Core::HID {
class HIDCore;
class EmulatedController;
}

namespace InputCommon {
class InputSubsystem;
}

class InputProfiles;
class PlayerControlPreview;

namespace GamepadMapper {

class ConfigureSinglePlayer : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureSinglePlayer(Core::HID::HIDCore& hid_core_,
                                   InputCommon::InputSubsystem* input_subsystem_,
                                   InputProfiles* profiles_,
                                   ControllerLayoutStyle initial_style = ControllerLayoutStyle::AutoDetect,
                                   QWidget* parent = nullptr);
    ~ConfigureSinglePlayer() override;

    void ApplyConfiguration();
    void SetLayoutStyle(ControllerLayoutStyle style);

private:
    void SetupLayout();
    void ConnectSignals();
    void LoadConfiguration();
    void UpdateInputDeviceCombobox();
    void UpdateInputProfilesCombobox();
    void UpdateLabelsForCurrentStyle();
    void UpdateButtonTextLabels();
    void HandleClick(QPushButton* button, std::size_t button_id,
                     std::function<void(const Common::ParamPackage&)> new_input_setter,
                     InputCommon::Polling::InputType type);
    void SetPollingResult(const Common::ParamPackage& params, bool abort);
    bool IsInputAcceptable(const Common::ParamPackage& params) const;

    void OnDeviceChanged(int index);
    void OnStyleChanged(int index);
    void OnNewProfile();
    void OnSaveProfile();
    void OnDeleteProfile();
    void OnLoadProfile(int index);
    void RestoreDefaults();
    void ClearAll();

    Core::HID::HIDCore& hid_core;
    InputCommon::InputSubsystem* input_subsystem;
    InputProfiles* profiles;
    Core::HID::EmulatedController* emulated_controller{nullptr};
    ControllerLayoutStyle current_style{ControllerLayoutStyle::AutoDetect};

    // UI Widgets
    QComboBox* combo_devices{nullptr};
    QComboBox* combo_styles{nullptr};
    QComboBox* combo_profiles{nullptr};
    QPushButton* btn_profile_save{nullptr};
    QPushButton* btn_profile_new{nullptr};
    QPushButton* btn_profile_delete{nullptr};

    // Controller Preview Frame
    PlayerControlPreview* controller_frame{nullptr};

    // Action/Button Mapping Buttons
    // Face Buttons
    QLabel* lbl_face_a{nullptr};
    QPushButton* btn_face_a{nullptr};
    QLabel* lbl_face_b{nullptr};
    QPushButton* btn_face_b{nullptr};
    QLabel* lbl_face_x{nullptr};
    QPushButton* btn_face_x{nullptr};
    QLabel* lbl_face_y{nullptr};
    QPushButton* btn_face_y{nullptr};

    // Shoulders & Triggers
    QLabel* lbl_l1{nullptr};
    QPushButton* btn_l1{nullptr};
    QLabel* lbl_r1{nullptr};
    QPushButton* btn_r1{nullptr};
    QLabel* lbl_l2{nullptr};
    QPushButton* btn_l2{nullptr};
    QLabel* lbl_r2{nullptr};
    QPushButton* btn_r2{nullptr};

    // Menu / Center Buttons
    QLabel* lbl_start{nullptr};
    QPushButton* btn_start{nullptr};
    QLabel* lbl_select{nullptr};
    QPushButton* btn_select{nullptr};

    // D-Pad
    QLabel* lbl_dpad_up{nullptr};
    QPushButton* btn_dpad_up{nullptr};
    QLabel* lbl_dpad_down{nullptr};
    QPushButton* btn_dpad_down{nullptr};
    QLabel* lbl_dpad_left{nullptr};
    QPushButton* btn_dpad_left{nullptr};
    QLabel* lbl_dpad_right{nullptr};
    QPushButton* btn_dpad_right{nullptr};

    // Sticks
    QLabel* lbl_l3{nullptr};
    QPushButton* btn_l3{nullptr};
    QPushButton* btn_lstick_up{nullptr};
    QPushButton* btn_lstick_down{nullptr};
    QPushButton* btn_lstick_left{nullptr};
    QPushButton* btn_lstick_right{nullptr};

    QLabel* lbl_r3{nullptr};
    QPushButton* btn_r3{nullptr};
    QPushButton* btn_rstick_up{nullptr};
    QPushButton* btn_rstick_down{nullptr};
    QPushButton* btn_rstick_left{nullptr};
    QPushButton* btn_rstick_right{nullptr};

    // Deadzones
    QSlider* slider_l_deadzone{nullptr};
    QSlider* slider_r_deadzone{nullptr};

    // Bottom tools
    QPushButton* btn_restore_defaults{nullptr};
    QPushButton* btn_clear_all{nullptr};

    // Polling & Update Timers
    std::unique_ptr<QTimer> timeout_timer;
    std::unique_ptr<QTimer> poll_timer;
    std::unique_ptr<QTimer> pump_timer;
    std::optional<std::function<void(const Common::ParamPackage&)>> input_setter;
};

} // namespace GamepadMapper
