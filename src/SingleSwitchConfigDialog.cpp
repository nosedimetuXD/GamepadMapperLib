// SPDX-License-Identifier: MIT
#include "GamepadMapper/SingleSwitchConfigDialog.h"
#include "GamepadMapper/GamepadManager.h"
#include "ui/configure_single_switch.h"
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

SingleSwitchConfigDialog::SingleSwitchConfigDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Configuración y Remapeo a Nintendo Switch"));
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

    static auto profiles = std::make_unique<InputProfiles>();

    config_widget = new ConfigureSingleSwitch(*hid_core, input_subsystem, profiles.get(), this);
    layout->addWidget(config_widget);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SingleSwitchConfigDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SingleSwitchConfigDialog::reject);
    layout->addWidget(buttonBox);

    auto* pumpTimer = new QTimer(this);
    connect(pumpTimer, &QTimer::timeout, this, [input_subsystem] {
        if (input_subsystem) {
            input_subsystem->PumpEvents();
        }
    });
    pumpTimer->start(16);
}

SingleSwitchConfigDialog::~SingleSwitchConfigDialog() = default;

void SingleSwitchConfigDialog::ApplyConfiguration() {
    if (config_widget) {
        config_widget->ApplyConfiguration();
    }
    QtConfig().SaveAllValues();
}

void SingleSwitchConfigDialog::accept() {
    ApplyConfiguration();
    QDialog::accept();
}

void SingleSwitchConfigDialog::reject() {
    QDialog::reject();
}

} // namespace GamepadMapper
