// SPDX-License-Identifier: MIT
#include "qt_config.h"
#include "core/input_common/main.h"
#include <QStandardPaths>
#include <QDir>

const std::array<int, Settings::NativeButton::NumButtons> QtConfig::default_buttons = {
    Qt::Key_C,     // A
    Qt::Key_X,     // B
    Qt::Key_V,     // X
    Qt::Key_Z,     // Y
    Qt::Key_F,     // LStick
    Qt::Key_G,     // RStick
    Qt::Key_Q,     // L
    Qt::Key_E,     // R
    Qt::Key_R,     // ZL
    Qt::Key_T,     // ZR
    Qt::Key_M,     // Plus
    Qt::Key_N,     // Minus
    Qt::Key_Left,  // DLeft
    Qt::Key_Up,    // DUp
    Qt::Key_Right, // DRight
    Qt::Key_Down,  // DDown
    Qt::Key_Q,     // SLLeft
    Qt::Key_E,     // SRLeft
    Qt::Key_Home,  // Home
    Qt::Key_Print, // Screenshot
    Qt::Key_Q,     // SLRight
    Qt::Key_E,     // SRRight
};

const std::array<int, Settings::NativeMotion::NumMotions> QtConfig::default_motions = {
    Qt::Key_7,
    Qt::Key_8,
};

const std::array<std::array<int, 4>, Settings::NativeAnalog::NumAnalogs> QtConfig::default_analogs = {{
    {Qt::Key_W, Qt::Key_S, Qt::Key_A, Qt::Key_D},
    {Qt::Key_I, Qt::Key_K, Qt::Key_J, Qt::Key_L},
}};

const std::array<int, 2> QtConfig::default_stick_mod = {
    Qt::Key_Shift,
    Qt::Key_Shift,
};

const std::array<int, 2> QtConfig::default_ringcon_analogs = {
    0, 0
};

QtConfig::QtConfig(const std::string& config_name, ConfigType config_type)
    : type(config_type) {
    const QString app_data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(app_data + "/input");
    QString path = app_data + "/" + QString::fromStdString(config_name) + ".ini";
    if (config_type == ConfigType::InputProfile) {
        path = app_data + "/input/" + QString::fromStdString(config_name) + ".ini";
    }
    settings = std::make_unique<QSettings>(path, QSettings::IniFormat);
}

QtConfig::~QtConfig() = default;

void QtConfig::ReloadAllValues() {
    for (std::size_t i = 0; i < 8; ++i) {
        ReadQtControlPlayerValues(i);
    }
}

void QtConfig::SaveAllValues() {
    for (std::size_t i = 0; i < 8; ++i) {
        SaveQtControlPlayerValues(i);
    }
    settings->sync();
}

void QtConfig::ReadQtControlPlayerValues(std::size_t player_index) {
    auto& player = Settings::values.players.GetValue()[player_index];
    QString prefix = QString("player_%1_").arg(player_index);

    player.connected = settings->value(prefix + "connected", player_index == 0).toBool();
    player.controller_type = static_cast<Settings::ControllerType>(
        settings->value(prefix + "type", static_cast<int>(Settings::ControllerType::ProController)).toInt());
    player.profile_name = settings->value(prefix + "profile_name", "").toString().toStdString();
    player.vibration_enabled = settings->value(prefix + "vibration_enabled", true).toBool();
    player.vibration_strength = settings->value(prefix + "vibration_strength", 100).toInt();

    for (int i = 0; i < Settings::NativeButton::NumButtons; ++i) {
        const std::string default_param = InputCommon::GenerateKeyboardParam(default_buttons[i]);
        QString key = prefix + QString::fromStdString(Settings::NativeButton::mapping[i]);
        player.buttons[i] = settings->value(key, QString::fromStdString(default_param)).toString().toStdString();
    }

    for (int i = 0; i < Settings::NativeAnalog::NumAnalogs; ++i) {
        const std::string default_param = InputCommon::GenerateAnalogParamFromKeys(
            default_analogs[i][0], default_analogs[i][1], default_analogs[i][2],
            default_analogs[i][3], default_stick_mod[i], 0.5f);
        QString key = prefix + QString::fromStdString(Settings::NativeAnalog::mapping[i]);
        player.analogs[i] = settings->value(key, QString::fromStdString(default_param)).toString().toStdString();
    }

    for (int i = 0; i < Settings::NativeMotion::NumMotions; ++i) {
        const std::string default_param = InputCommon::GenerateKeyboardParam(default_motions[i]);
        QString key = prefix + QString::fromStdString(Settings::NativeMotion::mapping[i]);
        player.motions[i] = settings->value(key, QString::fromStdString(default_param)).toString().toStdString();
    }
}

void QtConfig::SaveQtControlPlayerValues(std::size_t player_index) {
    const auto& player = Settings::values.players.GetValue()[player_index];
    QString prefix = QString("player_%1_").arg(player_index);

    settings->setValue(prefix + "connected", player.connected);
    settings->setValue(prefix + "type", static_cast<int>(player.controller_type));
    settings->setValue(prefix + "profile_name", QString::fromStdString(player.profile_name));
    settings->setValue(prefix + "vibration_enabled", player.vibration_enabled);
    settings->setValue(prefix + "vibration_strength", player.vibration_strength);

    for (int i = 0; i < Settings::NativeButton::NumButtons; ++i) {
        QString key = prefix + QString::fromStdString(Settings::NativeButton::mapping[i]);
        settings->setValue(key, QString::fromStdString(player.buttons[i]));
    }

    for (int i = 0; i < Settings::NativeAnalog::NumAnalogs; ++i) {
        QString key = prefix + QString::fromStdString(Settings::NativeAnalog::mapping[i]);
        settings->setValue(key, QString::fromStdString(player.analogs[i]));
    }

    for (int i = 0; i < Settings::NativeMotion::NumMotions; ++i) {
        QString key = prefix + QString::fromStdString(Settings::NativeMotion::mapping[i]);
        settings->setValue(key, QString::fromStdString(player.motions[i]));
    }
    settings->sync();
}
