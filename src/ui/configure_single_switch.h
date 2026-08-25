// SPDX-License-Identifier: MIT
#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTimer>
#include <array>
#include <memory>
#include <optional>
#include <functional>

#include "common/settings_input.h"
#include "common/param_package.h"
#include "core/hid_core/hid_types.h"
#include "input_common/main.h"

namespace Core::HID {
class HIDCore;
class EmulatedController;
}

class PlayerControlPreview;
class InputProfiles;

namespace GamepadMapper {

class ConfigureSingleSwitch : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureSingleSwitch(Core::HID::HIDCore& hid_core,
                                   InputCommon::InputSubsystem* input_subsystem,
                                   InputProfiles* profiles,
                                   QWidget* parent = nullptr);
    ~ConfigureSingleSwitch() override;

    void ApplyConfiguration();

private slots:
    void OnDeviceChanged(int index);
    void OnControllerTypeChanged(int index);
    void OnProfileChanged(int index);
    void OnNewProfile();
    void OnSaveProfile();
    void OnDeleteProfile();
    void RestoreDefaults();
    void ClearMappings();

private:
    void SetupLayout();
    void ConnectSignals();
    void LoadConfiguration();
    void UpdateInputDeviceCombobox();
    void UpdateInputProfilesCombobox();
    void UpdateButtonTextLabels();
    void UpdateControllerTypeVisibility();

    void HandleClick(QPushButton* button, std::size_t button_id,
                     std::function<void(const Common::ParamPackage&)> new_input_setter,
                     InputCommon::Polling::InputType type);
    void SetPollingResult(const Common::ParamPackage& params, bool abort);
    bool IsInputAcceptable(const Common::ParamPackage& params) const;

    Core::HID::HIDCore& hid_core;
    InputCommon::InputSubsystem* input_subsystem{nullptr};
    InputProfiles* profiles{nullptr};
    Core::HID::EmulatedController* emulated_controller{nullptr};

    std::optional<std::function<void(const Common::ParamPackage&)>> input_setter;

    // UI Top Bar Controls
    QComboBox* combo_devices{nullptr};
    QComboBox* combo_controller_type{nullptr};
    QComboBox* combo_profiles{nullptr};
    QPushButton* btn_save_profile{nullptr};
    QPushButton* btn_new_profile{nullptr};
    QPushButton* btn_delete_profile{nullptr};

    // Groups
    QGroupBox* left_stick_group{nullptr};
    QGroupBox* right_stick_group{nullptr};
    QGroupBox* dpad_group{nullptr};
    QGroupBox* face_group{nullptr};
    QGroupBox* left_triggers_group{nullptr};
    QGroupBox* right_triggers_group{nullptr};
    QGroupBox* joycon_sl_sr_group{nullptr};

    // Face Buttons (Switch A, B, X, Y)
    QPushButton* btn_face_a{nullptr};
    QPushButton* btn_face_b{nullptr};
    QPushButton* btn_face_x{nullptr};
    QPushButton* btn_face_y{nullptr};

    // D-Pad Buttons
    QPushButton* btn_dpad_up{nullptr};
    QPushButton* btn_dpad_down{nullptr};
    QPushButton* btn_dpad_left{nullptr};
    QPushButton* btn_dpad_right{nullptr};

    // Shoulders & Triggers
    QPushButton* btn_l{nullptr};
    QPushButton* btn_r{nullptr};
    QPushButton* btn_zl{nullptr};
    QPushButton* btn_zr{nullptr};
    QPushButton* btn_sl{nullptr};
    QPushButton* btn_sr{nullptr};

    // System Buttons
    QPushButton* btn_plus{nullptr};
    QPushButton* btn_minus{nullptr};
    QPushButton* btn_home{nullptr};
    QPushButton* btn_capture{nullptr};

    // Sticks
    QPushButton* btn_lstick_up{nullptr};
    QPushButton* btn_lstick_down{nullptr};
    QPushButton* btn_lstick_left{nullptr};
    QPushButton* btn_lstick_right{nullptr};
    QPushButton* btn_l3{nullptr};

    QPushButton* btn_rstick_up{nullptr};
    QPushButton* btn_rstick_down{nullptr};
    QPushButton* btn_rstick_left{nullptr};
    QPushButton* btn_rstick_right{nullptr};
    QPushButton* btn_r3{nullptr};

    // Bottom Action Buttons
    QPushButton* btn_restore_defaults{nullptr};
    QPushButton* btn_clear_all{nullptr};

    // Central Visual Preview
    PlayerControlPreview* controller_frame{nullptr};

    // Timers
    std::unique_ptr<QTimer> timeout_timer;
    std::unique_ptr<QTimer> poll_timer;
    std::unique_ptr<QTimer> pump_timer;
};

} // namespace GamepadMapper
