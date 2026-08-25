// SPDX-License-Identifier: MIT
#include "input_profiles.h"
#include "qt_config.h"

#include <algorithm>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

namespace {

QString GetInputProfilesPath() {
    const QString generic_data = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QString path = generic_data + "/GamepadMapper/input";
    QDir().mkpath(path);
    return path;
}

bool ProfileExistsInFilesystem(std::string_view profile_name) {
    const QString path = GetInputProfilesPath() + "/" + QString::fromUtf8(profile_name.data(), static_cast<int>(profile_name.size())) + ".ini";
    return QFile::exists(path);
}

} // namespace

InputProfiles::InputProfiles() {
    const QString input_dir_path = GetInputProfilesPath();
    QDir dir(input_dir_path);
    if (!dir.exists()) {
        dir.mkpath(".");
        return;
    }

    const QStringList files = dir.entryList(QStringList() << "*.ini", QDir::Files);
    for (const QString& file : files) {
        const QString base_name = QFileInfo(file).completeBaseName();
        const std::string name = base_name.toStdString();
        if (IsProfileNameValid(name)) {
            map_profiles.insert_or_assign(
                name,
                std::make_unique<QtConfig>(name, QtConfig::ConfigType::InputProfile));
        }
    }
}

InputProfiles::~InputProfiles() = default;

std::vector<std::string> InputProfiles::GetInputProfileNames() {
    std::vector<std::string> profile_names;
    profile_names.reserve(map_profiles.size());

    auto it = map_profiles.cbegin();
    while (it != map_profiles.cend()) {
        const auto& [profile_name, config] = *it;
        if (!ProfileExistsInFilesystem(profile_name)) {
            it = map_profiles.erase(it);
            continue;
        }

        profile_names.push_back(profile_name);
        ++it;
    }

    std::stable_sort(profile_names.begin(), profile_names.end());
    return profile_names;
}

bool InputProfiles::IsProfileNameValid(std::string_view profile_name) {
    return profile_name.find_first_of("<>:;\"/\\|,.!?*") == std::string::npos;
}

bool InputProfiles::CreateProfile(const std::string& profile_name, std::size_t player_index) {
    if (ProfileExistsInMap(profile_name)) {
        return false;
    }

    map_profiles.insert_or_assign(
        profile_name, std::make_unique<QtConfig>(profile_name, QtConfig::ConfigType::InputProfile));

    return SaveProfile(profile_name, player_index);
}

bool InputProfiles::DeleteProfile(const std::string& profile_name) {
    if (!ProfileExistsInMap(profile_name)) {
        return false;
    }

    const QString path = GetInputProfilesPath() + "/" + QString::fromStdString(profile_name) + ".ini";
    QFile::remove(path);
    map_profiles.erase(profile_name);

    return true;
}

bool InputProfiles::LoadProfile(const std::string& profile_name, std::size_t player_index) {
    if (!ProfileExistsInMap(profile_name)) {
        return false;
    }

    if (!ProfileExistsInFilesystem(profile_name)) {
        map_profiles.erase(profile_name);
        return false;
    }

    map_profiles[profile_name]->ReadQtControlPlayerValues(player_index);
    return true;
}

bool InputProfiles::SaveProfile(const std::string& profile_name, std::size_t player_index) {
    if (!ProfileExistsInMap(profile_name)) {
        return false;
    }

    map_profiles[profile_name]->SaveQtControlPlayerValues(player_index);
    return true;
}

bool InputProfiles::ProfileExistsInMap(const std::string& profile_name) const {
    return map_profiles.find(profile_name) != map_profiles.end();
}
