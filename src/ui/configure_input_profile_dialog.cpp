// SPDX-License-Identifier: GPL-2.0-or-later
#include "ui_configure_input_profile_dialog.h"
#include "configure_input_player.h"
#include "configure_input_profile_dialog.h"
#include "core/hid_core/hid_core.h"

ConfigureInputProfileDialog::ConfigureInputProfileDialog(
    QWidget* parent, InputCommon::InputSubsystem* input_subsystem, InputProfiles* profiles,
    Core::HID::HIDCore& hid_core)
    : QDialog(parent), ui(std::make_unique<Ui::ConfigureInputProfileDialog>()),
      profile_widget(new ConfigureInputPlayer(this, 9, nullptr, input_subsystem, profiles,
                                              hid_core, false, false)) {
    ui->setupUi(this);

    ui->controllerLayout->addWidget(profile_widget);

    connect(ui->clear_all_button, &QPushButton::clicked, this,
            [this] { profile_widget->ClearAll(); });
    connect(ui->restore_defaults_button, &QPushButton::clicked, this,
            [this] { profile_widget->RestoreDefaults(); });

    RetranslateUI();
}

ConfigureInputProfileDialog::~ConfigureInputProfileDialog() = default;

void ConfigureInputProfileDialog::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QDialog::changeEvent(event);
}

void ConfigureInputProfileDialog::RetranslateUI() {
    ui->retranslateUi(this);
}
