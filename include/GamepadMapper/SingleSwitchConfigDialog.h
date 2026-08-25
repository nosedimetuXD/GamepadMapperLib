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

class ConfigureSingleSwitch;

class SingleSwitchConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit SingleSwitchConfigDialog(QWidget* parent = nullptr);
    ~SingleSwitchConfigDialog() override;

    void ApplyConfiguration();

public slots:
    void accept() override;
    void reject() override;

private:
    ConfigureSingleSwitch* config_widget{nullptr};
};

} // namespace GamepadMapper
