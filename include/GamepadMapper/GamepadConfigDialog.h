// SPDX-License-Identifier: MIT
#pragma once

#include <memory>
#include <QWidget>
#include <QDialog>

class ConfigureInput;

namespace GamepadMapper {

class GamepadConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit GamepadConfigDialog(QWidget* parent = nullptr);
    ~GamepadConfigDialog() override;

    /// Apply and save configuration changes to disk
    void ApplyConfiguration();

public slots:
    void accept() override;
    void reject() override;

private:
    ConfigureInput* config_widget = nullptr;
};

/// Convenience function to show the dialog modally
inline bool ShowGamepadConfiguration(QWidget* parent = nullptr) {
    GamepadConfigDialog dialog(parent);
    return dialog.exec() == QDialog::Accepted;
}

} // namespace GamepadMapper
