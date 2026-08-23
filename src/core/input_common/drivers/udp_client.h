// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <string>
#include <vector>
#include "common/common_types.h"
#include "input_common/input_engine.h"

namespace InputCommon {

class UDPClient final : public InputEngine {
public:
    explicit UDPClient(std::string input_engine_);
    ~UDPClient() override;

    void ReloadSockets();

    std::vector<Common::ParamPackage> GetInputDevices() const override;
    ButtonMapping GetButtonMappingForDevice(const Common::ParamPackage& params) override;
    AnalogMapping GetAnalogMappingForDevice(const Common::ParamPackage& params) override;
    MotionMapping GetMotionMappingForDevice(const Common::ParamPackage& params) override;
    Common::Input::ButtonNames GetUIName(const Common::ParamPackage& params) const override;
    bool IsStickInverted(const Common::ParamPackage& params) override;
};

} // namespace InputCommon
