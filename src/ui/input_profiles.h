// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <ankerl/unordered_dense.h>

#include "qt_config.h"

class InputProfiles {
public:
    explicit InputProfiles();
    virtual ~InputProfiles();

    std::vector<std::string> GetInputProfileNames();

    static bool IsProfileNameValid(std::string_view profile_name);

    bool CreateProfile(const std::string& profile_name, std::size_t player_index);
    bool DeleteProfile(const std::string& profile_name);
    bool LoadProfile(const std::string& profile_name, std::size_t player_index);
    bool SaveProfile(const std::string& profile_name, std::size_t player_index);

private:
    bool ProfileExistsInMap(const std::string& profile_name) const;

    ankerl::unordered_dense::map<std::string, std::unique_ptr<QtConfig>> map_profiles;
};
