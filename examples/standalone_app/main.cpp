// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QStyleFactory>
#include "GamepadMapper/GamepadManager.h"
#include "GamepadMapper/GamepadConfigDialog.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Gamepad Mapper");
    app.setStyle(QStyleFactory::create("Fusion"));

    auto& manager = GamepadMapper::GamepadManager::Instance();
    if (!manager.Initialize()) {
        return 1;
    }

    GamepadMapper::GamepadConfigDialog dialog;
    dialog.show();

    const int result = app.exec();
    manager.Shutdown();
    return result;
}
