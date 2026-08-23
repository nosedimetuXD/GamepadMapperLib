// SPDX-License-Identifier: MIT
#include <iostream>
#include <thread>
#include <chrono>
#include "GamepadMapper/GamepadManager.h"

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " GamepadMapperLib - Game Polling Demo   " << std::endl;
    std::cout << "========================================" << std::endl;

    auto& manager = GamepadMapper::GamepadManager::Instance();
    if (!manager.Initialize()) {
        std::cerr << "Failed to initialize GamepadManager!" << std::endl;
        return 1;
    }

    std::cout << "GamepadManager initialized successfully." << std::endl;
    std::cout << "Polling Player 1 state at 60 FPS... (Press Ctrl+C to exit)" << std::endl;

    int frame = 0;
    while (true) {
        manager.Update();

        if (manager.IsConnected(0)) {
            auto stick = manager.GetStick(0, GamepadMapper::Stick::Left);
            bool btn_a = manager.IsButtonPressed(0, GamepadMapper::Button::A);
            bool btn_b = manager.IsButtonPressed(0, GamepadMapper::Button::B);
            bool btn_x = manager.IsButtonPressed(0, GamepadMapper::Button::X);
            bool btn_y = manager.IsButtonPressed(0, GamepadMapper::Button::Y);

            if (frame % 30 == 0) {
                std::cout << "\r[P1] LeftStick: (" << stick.x << ", " << stick.y << ")"
                          << " | A: " << (btn_a ? "ON " : "OFF")
                          << " | B: " << (btn_b ? "ON " : "OFF")
                          << " | X: " << (btn_x ? "ON " : "OFF")
                          << " | Y: " << (btn_y ? "ON " : "OFF")
                          << "        " << std::flush;
            }
        } else {
            if (frame % 60 == 0) {
                std::cout << "\r[P1] Not connected. Connect a controller or map keyboard.    " << std::flush;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        frame++;
    }

    manager.Shutdown();
    return 0;
}
