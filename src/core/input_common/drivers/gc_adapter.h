// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "input_common/input_engine.h"

#ifdef ENABLE_LIBUSB

#include <array>
#include <memory>
#include <string>
#include <thread>
#include "common/polyfill_thread.h"

struct libusb_context;
struct libusb_device;
struct libusb_device_handle;

namespace InputCommon {

class LibUSBContext;
class LibUSBDeviceHandle;

class GCAdapter : public InputEngine {
public:
    explicit GCAdapter(std::string input_engine_);
    ~GCAdapter() override;

    Common::Input::DriverResult SetVibration(
        const PadIdentifier& identifier, const Common::Input::VibrationStatus& vibration) override;

    bool IsVibrationEnabled(const PadIdentifier& identifier) override;

    std::vector<Common::ParamPackage> GetInputDevices() const override;
    ButtonMapping GetButtonMappingForDevice(const Common::ParamPackage& params) override;
    AnalogMapping GetAnalogMappingForDevice(const Common::ParamPackage& params) override;
    Common::Input::ButtonNames GetUIName(const Common::ParamPackage& params) const override;
    bool IsStickInverted(const Common::ParamPackage& params) override;

private:
    void AdapterInputThread();
    bool CheckDeviceAccess();
    bool Setup();
    void Reset();

    std::shared_ptr<LibUSBContext> libusb_ctx;
    std::unique_ptr<LibUSBDeviceHandle> libusb_handle;
    std::jthread input_thread;
    bool restart_scan = false;
};

} // namespace InputCommon

#else

namespace InputCommon {

class GCAdapter : public InputEngine {
public:
    explicit GCAdapter(std::string input_engine_) : InputEngine(std::move(input_engine_)) {}
    ~GCAdapter() override = default;

    Common::Input::DriverResult SetVibration(
        const PadIdentifier&, const Common::Input::VibrationStatus&) override {
        return Common::Input::DriverResult::NotSupported;
    }

    bool IsVibrationEnabled(const PadIdentifier&) override {
        return false;
    }

    std::vector<Common::ParamPackage> GetInputDevices() const override {
        return {};
    }

    ButtonMapping GetButtonMappingForDevice(const Common::ParamPackage&) override {
        return {};
    }

    AnalogMapping GetAnalogMappingForDevice(const Common::ParamPackage&) override {
        return {};
    }

    Common::Input::ButtonNames GetUIName(const Common::ParamPackage&) const override {
        return Common::Input::ButtonNames::Invalid;
    }

    bool IsStickInverted(const Common::ParamPackage&) override {
        return false;
    }
};

} // namespace InputCommon

#endif
