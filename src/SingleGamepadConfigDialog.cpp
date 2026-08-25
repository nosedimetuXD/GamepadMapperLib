// SPDX-License-Identifier: MIT
#include "GamepadMapper/SingleGamepadConfigDialog.h"
#include "GamepadMapper/GamepadManager.h"
#include "ui/configure_single_player.h"
#include "ui/input_profiles.h"
#include "core/hid_core/hid_core.h"
#include "core/input_common/main.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QTimer>
#include <QIcon>
#include <QShortcut>
#include <QKeySequence>

namespace GamepadMapper {

SingleGamepadConfigDialog::SingleGamepadConfigDialog(QWidget* parent,
                                                     ControllerLayoutStyle preferred_style)
    : QDialog(parent) {
    setWindowTitle(tr("Configuración y Remapeo de Mando"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/app_icon.png")));
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    resize(1200, 800);
    setMinimumSize(950, 600);

    // F11 Shortcut for toggling Fullscreen mode
    auto* fullscreen_shortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(fullscreen_shortcut, &QShortcut::activated, this, [this] {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);

    auto& manager = GamepadManager::Instance();
    manager.Initialize();

    auto* hid_core = static_cast<Core::HID::HIDCore*>(manager.GetHIDCore());
    auto* input_subsystem = static_cast<InputCommon::InputSubsystem*>(manager.GetInputSubsystem());

    // Allocate InputProfiles helper
    static auto profiles = std::make_unique<InputProfiles>();

    config_widget = new ConfigureSinglePlayer(*hid_core, input_subsystem, profiles.get(), preferred_style, this);
    layout->addWidget(config_widget);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SingleGamepadConfigDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SingleGamepadConfigDialog::reject);
    layout->addWidget(buttonBox);

    auto* pumpTimer = new QTimer(this);
    connect(pumpTimer, &QTimer::timeout, this, [input_subsystem] {
        if (input_subsystem) {
            input_subsystem->PumpEvents();
        }
    });
    pumpTimer->start(16);
}

SingleGamepadConfigDialog::~SingleGamepadConfigDialog() = default;

void SingleGamepadConfigDialog::ApplyConfiguration() {
    if (config_widget) {
        config_widget->ApplyConfiguration();
        auto* hid_core = static_cast<Core::HID::HIDCore*>(GamepadManager::Instance().GetHIDCore());
        if (hid_core) {
            hid_core->ReloadInputDevices();
        }
    }
}

void SingleGamepadConfigDialog::accept() {
    ApplyConfiguration();
    QDialog::accept();
}

void SingleGamepadConfigDialog::reject() {
    QDialog::reject();
}

} // namespace GamepadMapper
