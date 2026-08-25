# GamepadMapperLib

**GamepadMapperLib** is a standalone, modern C++20 gamepad and controller mapping library with an embeddable Qt GUI configuration dialog and multi-driver hardware abstraction (SDL3, Joy-Con, Keyboard/Mouse, Virtual Pads).

Extracted and modularized from the **Eden** emulator input subsystem.

---

## 🌟 Features

- **Multi-controller support**: Xbox (XInput), PlayStation (DualShock 4 / DualSense), Nintendo Switch (Pro Controller & Joy-Cons), generic DirectInput, Keyboard, and Mouse.
- **Up to 8 simultaneous players**.
- **Interactive Qt Configuration Dialog**: Visual controller mapping, stick deadzone/range calibration, profile management, and input polling.
- **Profile system**: Save and load controller configurations via `.ini` profiles.
- **Hardware rumble / haptic feedback** support.
- **Modern C++20 API**: Clean singleton interface (`GamepadManager`) for 60 FPS polling and input query.

---

## 📁 Project Structure

```
GamepadMapperLib/
├── include/GamepadMapper/           # Clean Public C++ API
│   ├── GamepadManager.h             # Core input polling and management
│   ├── GamepadTypes.h               # Enums (Button, Stick, Trigger) and State structs
│   ├── GamepadConfigDialog.h        # Multi-player Qt Modal Dialog
│   └── SingleGamepadConfigDialog.h  # Single-Player Remapper Dialog (FC 26 style)
├── src/                             # Subsystem & Drivers
│   ├── common/                      # Math, settings, logging, time, CPU features
│   ├── core/
│   │   ├── hid_core/                # Emulated controller logic, calibration, styles
│   │   └── input_common/            # SDL3 driver, Joy-Con driver, Keyboard/Mouse
│   └── ui/                          # Qt configuration forms, single player & preview
├── examples/
│   ├── single_player_mapper/        # SingleGamepadMapperApp (FC 26 style single controller)
│   ├── standalone_app/              # Multi-player GamepadMapperApp GUI executable
│   └── game_demo/                   # Console 60 FPS game polling demo
├── res/                             # Controller icons & overlay QRC assets
├── CMakeLists.txt
└── build.bat
```

---

## 🚀 Quick Start (C++ Integration)

### 1. Include Headers
```cpp
#include <GamepadMapper/GamepadManager.h>
#include <GamepadMapper/GamepadTypes.h>
```

### 2. Initialize
```cpp
auto& gamepad = GamepadMapper::GamepadManager::Instance();
gamepad.Initialize();
```

### 3. Update & Poll in your Game Loop
```cpp
while (game_running) {
    // 1. Pump hardware events and update internal controller states
    gamepad.Update();

    // 2. Poll Player 1 (index 0)
    if (gamepad.IsConnected(0)) {
        // Query buttons
        if (gamepad.IsButtonPressed(0, GamepadMapper::Button::A)) {
            // Jump / Accept action
        }

        // Query analog sticks (-1.0f to 1.0f)
        GamepadMapper::StickState left_stick = gamepad.GetStick(0, GamepadMapper::Stick::Left);
        float move_x = left_stick.x;
        float move_y = left_stick.y;

        // Query analog triggers (0.0f to 1.0f)
        GamepadMapper::TriggerState right_trigger = gamepad.GetTrigger(0, GamepadMapper::Trigger::Right);

        // Or take a full snapshot
        GamepadMapper::GamepadState state = gamepad.GetState(0);
    }
}

gamepad.Shutdown();
```

### 4. Open the Mapping GUI
```cpp
#include <GamepadMapper/GamepadConfigDialog.h>

// Launch the visual mapping modal from your game's settings menu
GamepadMapper::GamepadConfigDialog dialog(parent_window);
if (dialog.exec() == QDialog::Accepted) {
    // Settings are automatically saved and applied
}
```

---

## 🛠️ Building

### Requirements
- C++20 Compiler (MSVC 2022/2026, GCC 12+, Clang 15+)
- CMake 3.22+
- Qt 6 (Core, Gui, Widgets)
- Ninja or Visual Studio

### Build Commands (Windows)
```cmd
build.bat
```
Or with CMake:
```cmd
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## 📄 License
GPL-3.0-or-later / MIT (matching upstream Eden & Citra input subsystem licenses).
