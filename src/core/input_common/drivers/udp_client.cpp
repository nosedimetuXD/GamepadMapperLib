// SPDX-License-Identifier: GPL-2.0-or-later
#include "input_common/drivers/udp_client.h"

namespace InputCommon {

UDPClient::UDPClient(std::string input_engine_) : InputEngine(std::move(input_engine_)) {}

UDPClient::~UDPClient() = default;

void UDPClient::ReloadSockets() {}

std::vector<Common::ParamPackage> UDPClient::GetInputDevices() const {
    return {};
}

ButtonMapping UDPClient::GetButtonMappingForDevice(const Common::ParamPackage&) {
    return {};
}

AnalogMapping UDPClient::GetAnalogMappingForDevice(const Common::ParamPackage&) {
    return {};
}

MotionMapping UDPClient::GetMotionMappingForDevice(const Common::ParamPackage&) {
    return {};
}

Common::Input::ButtonNames UDPClient::GetUIName(const Common::ParamPackage&) const {
    return Common::Input::ButtonNames::Invalid;
}

bool UDPClient::IsStickInverted(const Common::ParamPackage&) {
    return false;
}

} // namespace InputCommon
