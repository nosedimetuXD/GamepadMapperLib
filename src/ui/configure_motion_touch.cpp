// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2018 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <sstream>

#include <QCloseEvent>
#include <QMessageBox>
#include <QStringListModel>

#include "common/logging.h"
#include "common/settings.h"
#include "common/param_package.h"
#include "input_common/drivers/udp_client.h"
#include "input_common/main.h"
#include "ui_configure_motion_touch.h"
#include "configure_motion_touch.h"

CalibrationConfigurationDialog::CalibrationConfigurationDialog(QWidget* parent,
                                                               const std::string& host, u16 port)
    : QDialog(parent) {
    layout = new QVBoxLayout(this);
    status_label = new QLabel(tr("Calibration completed."));
    cancel_button = new QPushButton(tr("OK"));
    connect(cancel_button, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(status_label);
    layout->addWidget(cancel_button);
}

CalibrationConfigurationDialog::~CalibrationConfigurationDialog() = default;

void CalibrationConfigurationDialog::UpdateLabelText(const QString& text) {
    status_label->setText(text);
}

void CalibrationConfigurationDialog::UpdateButtonText(const QString& text) {
    cancel_button->setText(text);
}

ConfigureMotionTouch::ConfigureMotionTouch(QWidget* parent,
                                           InputCommon::InputSubsystem* input_subsystem_)
    : QDialog(parent), input_subsystem{input_subsystem_},
      ui(std::make_unique<Ui::ConfigureMotionTouch>()) {
    ui->setupUi(this);
    SetConfiguration();
    UpdateUiDisplay();
    ConnectEvents();
}

ConfigureMotionTouch::~ConfigureMotionTouch() = default;

void ConfigureMotionTouch::SetConfiguration() {
    ui->udp_server->setText(QString::fromStdString("127.0.0.1"));
    ui->udp_port->setText(QString::number(26760));

    udp_server_list_model = new QStringListModel(this);
    udp_server_list_model->setStringList({});
    ui->udp_server_list->setModel(udp_server_list_model);

    std::stringstream ss(Settings::values.udp_input_servers.GetValue());
    std::string token;

    while (std::getline(ss, token, ',')) {
        const int row = udp_server_list_model->rowCount();
        udp_server_list_model->insertRows(row, 1);
        const QModelIndex index = udp_server_list_model->index(row);
        udp_server_list_model->setData(index, QString::fromStdString(token));
    }
}

void ConfigureMotionTouch::UpdateUiDisplay() {
    const QString cemuhook_udp = QStringLiteral("cemuhookudp");

    ui->touch_calibration->setVisible(true);
    ui->touch_calibration_config->setVisible(true);
    ui->touch_calibration_label->setVisible(true);
    ui->touch_calibration->setText(
        QStringLiteral("(%1, %2) - (%3, %4)").arg(min_x).arg(min_y).arg(max_x).arg(max_y));

    ui->udp_config_group_box->setVisible(true);
}

void ConfigureMotionTouch::ConnectEvents() {
    connect(ui->udp_test, &QPushButton::clicked, this, &ConfigureMotionTouch::OnCemuhookUDPTest);
    connect(ui->udp_add, &QPushButton::clicked, this, &ConfigureMotionTouch::OnUDPAddServer);
    connect(ui->udp_remove, &QPushButton::clicked, this, &ConfigureMotionTouch::OnUDPDeleteServer);
    connect(ui->touch_calibration_config, &QPushButton::clicked, this,
            &ConfigureMotionTouch::OnConfigureTouchCalibration);
    connect(ui->touch_from_button_config_btn, &QPushButton::clicked, this,
            &ConfigureMotionTouch::OnConfigureTouchFromButton);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
            &ConfigureMotionTouch::ApplyConfiguration);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, [this] {
        if (CanCloseDialog()) {
            reject();
        }
    });
}

void ConfigureMotionTouch::OnUDPAddServer() {
    // Validator for IP address
    const QRegularExpression re(QStringLiteral(
        R"re(^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)re"));
    bool ok;
    const QString port_text = ui->udp_port->text();
    const QString server_text = ui->udp_server->text();
    const QString server_string = tr("%1:%2").arg(server_text, port_text);
    const int port_number = port_text.toInt(&ok, 10);
    const int row = udp_server_list_model->rowCount();

    if (!ok) {
        QMessageBox::warning(this, tr("Eden"), tr("Port number has invalid characters"));
        return;
    }
    if (port_number < 0 || port_number > 65353) {
        QMessageBox::warning(this, tr("Eden"), tr("Port has to be in range 0 and 65353"));
        return;
    }
    if (!re.match(server_text).hasMatch()) {
        QMessageBox::warning(this, tr("Eden"), tr("IP address is not valid"));
        return;
    }
    // Search for duplicates
    for (const auto& item : udp_server_list_model->stringList()) {
        if (item == server_string) {
            QMessageBox::warning(this, tr("Eden"), tr("This UDP server already exists"));
            return;
        }
    }
    // Limit server count to 8
    if (row == 8) {
        QMessageBox::warning(this, tr("Eden"), tr("Unable to add more than 8 servers"));
        return;
    }

    udp_server_list_model->insertRows(row, 1);
    QModelIndex index = udp_server_list_model->index(row);
    udp_server_list_model->setData(index, server_string);
    ui->udp_server_list->setCurrentIndex(index);
}

void ConfigureMotionTouch::OnUDPDeleteServer() {
    udp_server_list_model->removeRows(ui->udp_server_list->currentIndex().row(), 1);
}

void ConfigureMotionTouch::OnCemuhookUDPTest() {
    ShowUDPTestResult(true);
}

void ConfigureMotionTouch::OnConfigureTouchCalibration() {
    CalibrationConfigurationDialog dialog(this, ui->udp_server->text().toStdString(),
                                          static_cast<u16>(ui->udp_port->text().toUInt()));
    dialog.exec();
}

void ConfigureMotionTouch::closeEvent(QCloseEvent* event) {
    if (CanCloseDialog()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void ConfigureMotionTouch::ShowUDPTestResult(bool result) {
    udp_test_in_progress = false;
    if (result) {
        QMessageBox::information(this, tr("Test Successful"),
                                 tr("Successfully received data from the server."));
    } else {
        QMessageBox::warning(this, tr("Test Failed"),
                             tr("Could not receive valid data from the server."));
    }
    ui->udp_test->setEnabled(true);
    ui->udp_test->setText(tr("Test"));
}

void ConfigureMotionTouch::OnConfigureTouchFromButton() {
}

bool ConfigureMotionTouch::CanCloseDialog() {
    if (udp_test_in_progress) {
        QMessageBox::warning(this, tr("Eden"),
                             tr("UDP Test or calibration configuration is in progress.<br>Please "
                                "wait for them to finish."));
        return false;
    }
    return true;
}

void ConfigureMotionTouch::ApplyConfiguration() {
    if (!CanCloseDialog()) {
        return;
    }

    Settings::values.udp_input_servers = GetUDPServerString();
    input_subsystem->ReloadInputDevices();

    accept();
}

std::string ConfigureMotionTouch::GetUDPServerString() const {
    QString input_servers;

    for (const auto& item : udp_server_list_model->stringList()) {
        input_servers += item;
        input_servers += QLatin1Char{','};
    }

    // Remove last comma
    input_servers.chop(1);
    return input_servers.toStdString();
}

