// SPDX-License-Identifier: MIT
#include "GamepadMapper/GamepadConfigDialog.h"
#include "GamepadMapper/GamepadManager.h"
#include "ui/configure_input.h"
#include "core/hid_core/hid_core.h"
#include "core/input_common/main.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QTimer>

namespace GamepadMapper {

GamepadConfigDialog::GamepadConfigDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Configuración de Mandos y Controles"));
    setMinimumSize(850, 600);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    auto& manager = GamepadManager::Instance();
    manager.Initialize();

    auto* hid_core = static_cast<Core::HID::HIDCore*>(manager.GetHIDCore());
    auto* input_subsystem = static_cast<InputCommon::InputSubsystem*>(manager.GetInputSubsystem());

    config_widget = new ConfigureInput(*hid_core, this);
    config_widget->Initialize(input_subsystem, 8);
    layout->addWidget(config_widget);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &GamepadConfigDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &GamepadConfigDialog::reject);
    layout->addWidget(buttonBox);

    auto* pumpTimer = new QTimer(this);
    connect(pumpTimer, &QTimer::timeout, this, [input_subsystem] {
        if (input_subsystem) {
            input_subsystem->PumpEvents();
        }
    });
    pumpTimer->start(16);
}

GamepadConfigDialog::~GamepadConfigDialog() = default;

void GamepadConfigDialog::ApplyConfiguration() {
    if (config_widget) {
        config_widget->ApplyConfiguration();
        auto* hid_core = static_cast<Core::HID::HIDCore*>(GamepadManager::Instance().GetHIDCore());
        if (hid_core) {
            hid_core->ReloadInputDevices();
        }
    }
}

void GamepadConfigDialog::accept() {
    ApplyConfiguration();
    QDialog::accept();
}

void GamepadConfigDialog::reject() {
    QDialog::reject();
}

} // namespace GamepadMapper
