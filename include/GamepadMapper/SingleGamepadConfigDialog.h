// SPDX-License-Identifier: MIT
#pragma once

#include <QDialog>
#include <memory>

namespace Core::HID {
class HIDCore;
}

namespace InputCommon {
class InputSubsystem;
}

namespace GamepadMapper {

enum class ControllerLayoutStyle {
    AutoDetect,
    Xbox,
    PlayStation,
    Nintendo,
    GenericPC,
};

class ConfigureSinglePlayer;

class SingleGamepadConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit SingleGamepadConfigDialog(QWidget* parent = nullptr,
                                       ControllerLayoutStyle preferred_style = ControllerLayoutStyle::AutoDetect);
    ~SingleGamepadConfigDialog() override;

    void ApplyConfiguration();

public slots:
    void accept() override;
    void reject() override;

private:
    ConfigureSinglePlayer* config_widget{nullptr};
};

} // namespace GamepadMapper
