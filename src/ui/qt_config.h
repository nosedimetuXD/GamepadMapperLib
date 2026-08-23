// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <QSettings>

#include "common/settings.h"

class QtConfig {
public:
    enum class ConfigType {
        GlobalConfig,
        InputProfile,
    };

    explicit QtConfig(const std::string& config_name = "gamepad_mapper",
                      ConfigType config_type = ConfigType::GlobalConfig);
    virtual ~QtConfig();

    void ReloadAllValues();
    void SaveAllValues();

    void ReadQtControlPlayerValues(std::size_t player_index);
    void SaveQtControlPlayerValues(std::size_t player_index);

    static const std::array<int, Settings::NativeButton::NumButtons> default_buttons;
    static const std::array<int, Settings::NativeMotion::NumMotions> default_motions;
    static const std::array<std::array<int, 4>, Settings::NativeAnalog::NumAnalogs> default_analogs;
    static const std::array<int, 2> default_stick_mod;
    static const std::array<int, 2> default_ringcon_analogs;

private:
    std::unique_ptr<QSettings> settings;
    ConfigType type;
};
