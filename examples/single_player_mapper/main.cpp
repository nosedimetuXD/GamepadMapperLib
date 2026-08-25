// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>
#include <QIcon>

#include "GamepadMapper/GamepadManager.h"
#include "GamepadMapper/SingleGamepadConfigDialog.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SingleGamepadMapperApp"));
    app.setOrganizationName(QStringLiteral("GamepadMapper"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app_icon.png")));

    // Set Modern Dark Theme
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::WindowText, QColor(220, 220, 220));
    darkPalette.setColor(QPalette::Base, QColor(20, 20, 20));
    darkPalette.setColor(QPalette::AlternateBase, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::ToolTipText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Text, QColor(220, 220, 220));
    darkPalette.setColor(QPalette::Button, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::ButtonText, QColor(220, 220, 220));
    darkPalette.setColor(QPalette::BrightText, QColor(255, 0, 0));
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
    app.setPalette(darkPalette);

    // Initialize Gamepad subsystem
    auto& manager = GamepadMapper::GamepadManager::Instance();
    manager.Initialize();

    GamepadMapper::SingleGamepadConfigDialog dialog;
    dialog.show();

    const int result = app.exec();
    manager.Shutdown();
    return result;
}
