// SPDX-License-Identifier: MIT
#include "GamepadMapper/GamepadMapperC.h"
#include "GamepadMapper/GamepadManager.h"
#include "GamepadMapper/GamepadConfigDialog.h"
#include "GamepadMapper/SingleGamepadConfigDialog.h"
#include "GamepadMapper/SingleSwitchConfigDialog.h"

#include <QApplication>
#include <QStyleFactory>
#include <cstring>
#include <memory>

static std::unique_ptr<QApplication> s_qt_app;

static void EnsureQtApp() {
    if (!QApplication::instance()) {
        QCoreApplication::addLibraryPath(QStringLiteral("C:/Qt/6.8.2/msvc2022_64/plugins"));
        QCoreApplication::addLibraryPath(QStringLiteral("."));
        QCoreApplication::addLibraryPath(QStringLiteral("plugins"));

        static int argc = 1;
        static char app_name[] = "GamepadMapperApp";
        static char* argv[] = { app_name, nullptr };
        s_qt_app = std::make_unique<QApplication>(argc, argv);
        s_qt_app->setStyle(QStyleFactory::create("Fusion"));
    }
}

extern "C" {

bool gamepad_initialize(void) {
    return GamepadMapper::GamepadManager::Instance().Initialize();
}

void gamepad_shutdown(void) {
    GamepadMapper::GamepadManager::Instance().Shutdown();
}

void gamepad_update(void) {
    GamepadMapper::GamepadManager::Instance().Update();
}

void gamepad_reload(void) {
    GamepadMapper::GamepadManager::Instance().Reload();
}

bool gamepad_is_connected(int player) {
    return GamepadMapper::GamepadManager::Instance().IsConnected(player);
}

int gamepad_get_type(int player) {
    return static_cast<int>(GamepadMapper::GamepadManager::Instance().GetType(player));
}

bool gamepad_is_button_pressed(int player, int button) {
    if (button < 0 || button >= static_cast<int>(GamepadMapper::Button::Count)) {
        return false;
    }
    return GamepadMapper::GamepadManager::Instance().IsButtonPressed(
        player, static_cast<GamepadMapper::Button>(button));
}

bool gamepad_get_stick(int player, int stick, float* out_x, float* out_y) {
    if (!out_x || !out_y || stick < 0 || stick >= static_cast<int>(GamepadMapper::Stick::Count)) {
        return false;
    }
    auto s = GamepadMapper::GamepadManager::Instance().GetStick(
        player, static_cast<GamepadMapper::Stick>(stick));
    *out_x = s.x;
    *out_y = s.y;
    return true;
}

bool gamepad_get_trigger(int player, int trigger, float* out_value, bool* out_pressed) {
    if (!out_value || !out_pressed || trigger < 0 || trigger >= static_cast<int>(GamepadMapper::Trigger::Count)) {
        return false;
    }
    auto t = GamepadMapper::GamepadManager::Instance().GetTrigger(
        player, static_cast<GamepadMapper::Trigger>(trigger));
    *out_value = t.value;
    *out_pressed = t.pressed;
    return true;
}

bool gamepad_get_motion(int player, GamepadMotionState* out_motion) {
    if (!out_motion) {
        return false;
    }
    auto m = GamepadMapper::GamepadManager::Instance().GetMotion(player);
    out_motion->accel_x = m.accel_x;
    out_motion->accel_y = m.accel_y;
    out_motion->accel_z = m.accel_z;
    out_motion->gyro_x = m.gyro_x;
    out_motion->gyro_y = m.gyro_y;
    out_motion->gyro_z = m.gyro_z;
    out_motion->quat_w = m.quat_w;
    out_motion->quat_x = m.quat_x;
    out_motion->quat_y = m.quat_y;
    out_motion->quat_z = m.quat_z;
    return true;
}

bool gamepad_get_state(int player, GamepadFullState* out_state) {
    if (!out_state) {
        return false;
    }
    auto state = GamepadMapper::GamepadManager::Instance().GetState(player);
    out_state->is_connected = state.is_connected;
    out_state->type = static_cast<uint32_t>(state.type);
    out_state->buttons = state.buttons;
    out_state->left_stick.x = state.left_stick.x;
    out_state->left_stick.y = state.left_stick.y;
    out_state->right_stick.x = state.right_stick.x;
    out_state->right_stick.y = state.right_stick.y;
    out_state->left_trigger.value = state.left_trigger.value;
    out_state->left_trigger.pressed = state.left_trigger.pressed;
    out_state->right_trigger.value = state.right_trigger.value;
    out_state->right_trigger.pressed = state.right_trigger.pressed;
    out_state->motion.accel_x = state.motion.accel_x;
    out_state->motion.accel_y = state.motion.accel_y;
    out_state->motion.accel_z = state.motion.accel_z;
    out_state->motion.gyro_x = state.motion.gyro_x;
    out_state->motion.gyro_y = state.motion.gyro_y;
    out_state->motion.gyro_z = state.motion.gyro_z;
    out_state->motion.quat_w = state.motion.quat_w;
    out_state->motion.quat_x = state.motion.quat_x;
    out_state->motion.quat_y = state.motion.quat_y;
    out_state->motion.quat_z = state.motion.quat_z;
    out_state->battery_percentage = state.battery_percentage;
    out_state->is_charging = state.is_charging;
    return true;
}

void gamepad_set_vibration(int player, float low_frequency, float high_frequency) {
    GamepadMapper::GamepadManager::Instance().SetVibration(player, low_frequency, high_frequency);
}

bool gamepad_save_profile(int player, const char* profile_name) {
    if (!profile_name) return false;
    return GamepadMapper::GamepadManager::Instance().SaveProfile(player, profile_name);
}

bool gamepad_load_profile(int player, const char* profile_name) {
    if (!profile_name) return false;
    return GamepadMapper::GamepadManager::Instance().LoadProfile(player, profile_name);
}

bool gamepad_show_config_dialog(void* parent_window) {
    EnsureQtApp();
    GamepadMapper::GamepadConfigDialog dialog(static_cast<QWidget*>(parent_window));
    return dialog.exec() == QDialog::Accepted;
}

bool gamepad_show_single_config_dialog(void* parent_window) {
    EnsureQtApp();
    GamepadMapper::SingleGamepadConfigDialog dialog(static_cast<QWidget*>(parent_window));
    return dialog.exec() == QDialog::Accepted;
}

bool gamepad_show_single_switch_config_dialog(void* parent_window) {
    EnsureQtApp();
    GamepadMapper::SingleSwitchConfigDialog dialog(static_cast<QWidget*>(parent_window));
    return dialog.exec() == QDialog::Accepted;
}

} // extern "C"
