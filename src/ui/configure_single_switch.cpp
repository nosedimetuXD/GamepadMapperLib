// SPDX-License-Identifier: MIT
#include "configure_single_switch.h"
#include "configure_input_player_widget.h"
#include "input_profiles.h"
#include "limitable_input_dialog.h"
#include "qt_config.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QKeySequence>
#include <algorithm>

#include "core/hid_core/frontend/emulated_controller.h"
#include "core/hid_core/hid_core.h"
#include "input_common/main.h"

namespace GamepadMapper {

namespace {

QString GetKeyName(int key_code) {
    if (key_code == 0) return QStringLiteral("[none]");
    if (key_code == Qt::Key_Shift) return QStringLiteral("Shift");
    if (key_code == Qt::Key_Control) return QStringLiteral("Ctrl");
    if (key_code == Qt::Key_Alt) return QStringLiteral("Alt");
    if (key_code == Qt::Key_Meta) return QStringLiteral("Windows");
    return QKeySequence(key_code).toString();
}

QString ButtonToLabel(const Common::ParamPackage& param, InputCommon::InputSubsystem* input_subsystem) {
    if (param.Serialize().empty()) {
        return QObject::tr("[sin asignar]");
    }
    if (param.Get("engine", "") == "keyboard") {
        return GetKeyName(param.Get("code", 0));
    }
    if (param.Has("button")) {
        return QStringLiteral("Botón %1").arg(QString::fromStdString(param.Get("button", "")));
    }
    if (param.Has("axis")) {
        const auto direction = param.Get("direction", "");
        return QStringLiteral("Eje %1%2").arg(
            QString::fromStdString(param.Get("axis", "")),
            QString::fromStdString(direction));
    }
    if (param.Has("hat")) {
        return QStringLiteral("Hat %1").arg(QString::fromStdString(param.Get("direction", "")));
    }
    return QString::fromStdString(param.Serialize());
}

} // anonymous namespace

ConfigureSingleSwitch::ConfigureSingleSwitch(Core::HID::HIDCore& hid_core_,
                                             InputCommon::InputSubsystem* input_subsystem_,
                                             InputProfiles* profiles_,
                                             QWidget* parent)
    : QWidget(parent), hid_core{hid_core_}, input_subsystem{input_subsystem_}, profiles{profiles_},
      timeout_timer{std::make_unique<QTimer>()},
      poll_timer{std::make_unique<QTimer>()},
      pump_timer{std::make_unique<QTimer>()} {

    emulated_controller = hid_core.GetEmulatedController(Core::HID::NpadIdType::Player1);
    if (emulated_controller) {
        emulated_controller->EnableConfiguration();
        emulated_controller->Connect(true);
        emulated_controller->SetNpadStyleIndex(Core::HID::NpadStyleIndex::Fullkey);
    }

    SetupLayout();
    ConnectSignals();
    LoadConfiguration();

    pump_timer->start(16); // 60 FPS event loop for immediate gamepad responsiveness
}

ConfigureSingleSwitch::~ConfigureSingleSwitch() {
    if (emulated_controller) {
        emulated_controller->DisableConfiguration();
    }
}

void ConfigureSingleSwitch::SetupLayout() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(10);
    main_layout->setContentsMargins(10, 10, 10, 10);

    // ==========================================
    // Top Bar: Device, Switch Controller Type, Profiles
    // ==========================================
    auto* top_group = new QGroupBox(tr("Configuración de Mando de Nintendo Switch y Perfiles"), this);
    auto* top_layout = new QGridLayout(top_group);

    top_layout->addWidget(new QLabel(tr("Mando Físico / Entrada:"), this), 0, 0);
    combo_devices = new QComboBox(this);
    top_layout->addWidget(combo_devices, 0, 1);

    top_layout->addWidget(new QLabel(tr("Tipo de Mando Switch:"), this), 0, 2);
    combo_controller_type = new QComboBox(this);
    combo_controller_type->addItem(tr("Nintendo Switch Pro Controller"), static_cast<int>(Core::HID::NpadStyleIndex::Fullkey));
    combo_controller_type->addItem(tr("Joy-Con Duales"), static_cast<int>(Core::HID::NpadStyleIndex::JoyconDual));
    combo_controller_type->addItem(tr("Joy-Con Izquierdo"), static_cast<int>(Core::HID::NpadStyleIndex::JoyconLeft));
    combo_controller_type->addItem(tr("Joy-Con Derecho"), static_cast<int>(Core::HID::NpadStyleIndex::JoyconRight));
    combo_controller_type->addItem(tr("Mando de GameCube"), static_cast<int>(Core::HID::NpadStyleIndex::GameCube));
    combo_controller_type->addItem(tr("Modo Portátil (Handheld)"), static_cast<int>(Core::HID::NpadStyleIndex::Handheld));
    top_layout->addWidget(combo_controller_type, 0, 3);

    top_layout->addWidget(new QLabel(tr("Perfil:"), this), 1, 0);
    combo_profiles = new QComboBox(this);
    top_layout->addWidget(combo_profiles, 1, 1);

    btn_save_profile = new QPushButton(tr("Guardar"), this);
    btn_new_profile = new QPushButton(tr("Nuevo"), this);
    btn_delete_profile = new QPushButton(tr("Eliminar"), this);

    auto* profile_btn_layout = new QHBoxLayout();
    profile_btn_layout->addWidget(btn_save_profile);
    profile_btn_layout->addWidget(btn_new_profile);
    profile_btn_layout->addWidget(btn_delete_profile);
    top_layout->addLayout(profile_btn_layout, 1, 2, 1, 2);

    main_layout->addWidget(top_group);

    // ==========================================
    // Center Panels: Left (Sticks/D-Pad) | Center (Visual Switch Controller) | Right (Action Buttons/Triggers)
    // ==========================================
    auto* center_layout = new QHBoxLayout();
    center_layout->setSpacing(12);

    // --- Left Panel ---
    auto* left_panel = new QVBoxLayout();

    // Left Stick Group
    left_stick_group = new QGroupBox(tr("Stick Izquierdo (L-Stick)"), this);
    auto* left_stick_grid = new QGridLayout(left_stick_group);
    btn_lstick_up = new QPushButton(this);
    btn_lstick_down = new QPushButton(this);
    btn_lstick_left = new QPushButton(this);
    btn_lstick_right = new QPushButton(this);
    btn_l3 = new QPushButton(this);

    left_stick_grid->addWidget(new QLabel(tr("Arriba:"), this), 0, 0);
    left_stick_grid->addWidget(btn_lstick_up, 0, 1);
    left_stick_grid->addWidget(new QLabel(tr("Abajo:"), this), 1, 0);
    left_stick_grid->addWidget(btn_lstick_down, 1, 1);
    left_stick_grid->addWidget(new QLabel(tr("Izquierda:"), this), 2, 0);
    left_stick_grid->addWidget(btn_lstick_left, 2, 1);
    left_stick_grid->addWidget(new QLabel(tr("Derecha:"), this), 3, 0);
    left_stick_grid->addWidget(btn_lstick_right, 3, 1);
    left_stick_grid->addWidget(new QLabel(tr("Click Stick Izq ( L-Stick / L3 ):"), this), 4, 0);
    left_stick_grid->addWidget(btn_l3, 4, 1);
    left_panel->addWidget(left_stick_group);

    // D-Pad Group
    dpad_group = new QGroupBox(tr("Cruceta Direccional de Switch (D-Pad)"), this);
    auto* dpad_grid = new QGridLayout(dpad_group);
    btn_dpad_up = new QPushButton(this);
    btn_dpad_down = new QPushButton(this);
    btn_dpad_left = new QPushButton(this);
    btn_dpad_right = new QPushButton(this);

    dpad_grid->addWidget(new QLabel(tr("Arriba:"), this), 0, 0);
    dpad_grid->addWidget(btn_dpad_up, 0, 1);
    dpad_grid->addWidget(new QLabel(tr("Abajo:"), this), 1, 0);
    dpad_grid->addWidget(btn_dpad_down, 1, 1);
    dpad_grid->addWidget(new QLabel(tr("Izquierda:"), this), 2, 0);
    dpad_grid->addWidget(btn_dpad_left, 2, 1);
    dpad_grid->addWidget(new QLabel(tr("Derecha:"), this), 3, 0);
    dpad_grid->addWidget(btn_dpad_right, 3, 1);
    left_panel->addWidget(dpad_group);

    // Left Triggers & Shoulders
    left_triggers_group = new QGroupBox(tr("Gatillos Izquierdos de Switch"), this);
    auto* left_triggers_grid = new QGridLayout(left_triggers_group);
    btn_l = new QPushButton(this);
    btn_zl = new QPushButton(this);

    left_triggers_grid->addWidget(new QLabel(tr("Botón L:"), this), 0, 0);
    left_triggers_grid->addWidget(btn_l, 0, 1);
    left_triggers_grid->addWidget(new QLabel(tr("Gatillo ZL:"), this), 1, 0);
    left_triggers_grid->addWidget(btn_zl, 1, 1);
    left_panel->addWidget(left_triggers_group);

    center_layout->addLayout(left_panel, 1);

    // --- Middle Panel: Visual Switch Controller Preview ---
    auto* preview_group = new QGroupBox(tr("Vista Previa en Tiempo Real (Mando de Nintendo Switch)"), this);
    auto* preview_layout = new QVBoxLayout(preview_group);
    controller_frame = new PlayerControlPreview(preview_group);
    controller_frame->setMinimumSize(320, 240);
    controller_frame->SetVisualLayoutStyle(PlayerControlPreview::VisualLayoutStyle::Nintendo);
    if (emulated_controller) {
        controller_frame->SetController(emulated_controller);
    }
    preview_layout->addWidget(controller_frame);
    center_layout->addWidget(preview_group, 2);

    // --- Right Panel ---
    auto* right_panel = new QVBoxLayout();

    // Face Buttons Group (Switch A, B, X, Y)
    face_group = new QGroupBox(tr("Botones Principales de Switch (Acción)"), this);
    auto* face_grid = new QGridLayout(face_group);
    btn_face_a = new QPushButton(this);
    btn_face_b = new QPushButton(this);
    btn_face_x = new QPushButton(this);
    btn_face_y = new QPushButton(this);

    face_grid->addWidget(new QLabel(tr("Botón A (Derecha):"), this), 0, 0);
    face_grid->addWidget(btn_face_a, 0, 1);
    face_grid->addWidget(new QLabel(tr("Botón B (Inferior):"), this), 1, 0);
    face_grid->addWidget(btn_face_b, 1, 1);
    face_grid->addWidget(new QLabel(tr("Botón X (Superior):"), this), 2, 0);
    face_grid->addWidget(btn_face_x, 2, 1);
    face_grid->addWidget(new QLabel(tr("Botón Y (Izquierda):"), this), 3, 0);
    face_grid->addWidget(btn_face_y, 3, 1);
    right_panel->addWidget(face_group);

    // Right Stick Group
    right_stick_group = new QGroupBox(tr("Stick Derecho (R-Stick)"), this);
    auto* right_stick_grid = new QGridLayout(right_stick_group);
    btn_rstick_up = new QPushButton(this);
    btn_rstick_down = new QPushButton(this);
    btn_rstick_left = new QPushButton(this);
    btn_rstick_right = new QPushButton(this);
    btn_r3 = new QPushButton(this);

    right_stick_grid->addWidget(new QLabel(tr("Arriba:"), this), 0, 0);
    right_stick_grid->addWidget(btn_rstick_up, 0, 1);
    right_stick_grid->addWidget(new QLabel(tr("Abajo:"), this), 1, 0);
    right_stick_grid->addWidget(btn_rstick_down, 1, 1);
    right_stick_grid->addWidget(new QLabel(tr("Izquierda:"), this), 2, 0);
    right_stick_grid->addWidget(btn_rstick_left, 2, 1);
    right_stick_grid->addWidget(new QLabel(tr("Derecha:"), this), 3, 0);
    right_stick_grid->addWidget(btn_rstick_right, 3, 1);
    right_stick_grid->addWidget(new QLabel(tr("Click Stick Der ( R-Stick / R3 ):"), this), 4, 0);
    right_stick_grid->addWidget(btn_r3, 4, 1);
    right_panel->addWidget(right_stick_group);

    // Right Triggers & System Buttons
    right_triggers_group = new QGroupBox(tr("Gatillos Derechos y Sistema de Switch"), this);
    auto* right_triggers_grid = new QGridLayout(right_triggers_group);
    btn_r = new QPushButton(this);
    btn_zr = new QPushButton(this);
    btn_plus = new QPushButton(this);
    btn_minus = new QPushButton(this);
    btn_home = new QPushButton(this);
    btn_capture = new QPushButton(this);

    right_triggers_grid->addWidget(new QLabel(tr("Botón R:"), this), 0, 0);
    right_triggers_grid->addWidget(btn_r, 0, 1);
    right_triggers_grid->addWidget(new QLabel(tr("Gatillo ZR:"), this), 1, 0);
    right_triggers_grid->addWidget(btn_zr, 1, 1);
    right_triggers_grid->addWidget(new QLabel(tr("Botón Más ( + ):"), this), 2, 0);
    right_triggers_grid->addWidget(btn_plus, 2, 1);
    right_triggers_grid->addWidget(new QLabel(tr("Botón Menos ( - ):"), this), 3, 0);
    right_triggers_grid->addWidget(btn_minus, 3, 1);
    right_triggers_grid->addWidget(new QLabel(tr("Botón Inicio ( Home ):"), this), 4, 0);
    right_triggers_grid->addWidget(btn_home, 4, 1);
    right_triggers_grid->addWidget(new QLabel(tr("Botón Captura ( Screenshot ):"), this), 5, 0);
    right_triggers_grid->addWidget(btn_capture, 5, 1);
    right_panel->addWidget(right_triggers_group);

    // Joy-Con SL / SR Buttons Group (Optional)
    joycon_sl_sr_group = new QGroupBox(tr("Botones Laterales Joy-Con (SL / SR)"), this);
    auto* sl_sr_grid = new QGridLayout(joycon_sl_sr_group);
    btn_sl = new QPushButton(this);
    btn_sr = new QPushButton(this);
    sl_sr_grid->addWidget(new QLabel(tr("Botón SL:"), this), 0, 0);
    sl_sr_grid->addWidget(btn_sl, 0, 1);
    sl_sr_grid->addWidget(new QLabel(tr("Botón SR:"), this), 1, 0);
    sl_sr_grid->addWidget(btn_sr, 1, 1);
    right_panel->addWidget(joycon_sl_sr_group);
    joycon_sl_sr_group->setVisible(false);

    center_layout->addLayout(right_panel, 1);
    main_layout->addLayout(center_layout);

    // ==========================================
    // Bottom Action Bar: Reset Defaults, Clear Mappings
    // ==========================================
    auto* bottom_bar = new QHBoxLayout();
    btn_restore_defaults = new QPushButton(tr("Restablecer por Defecto"), this);
    btn_clear_all = new QPushButton(tr("Limpiar Mapeos"), this);

    bottom_bar->addWidget(btn_restore_defaults);
    bottom_bar->addWidget(btn_clear_all);
    bottom_bar->addStretch();
    main_layout->addLayout(bottom_bar);
}

void ConfigureSingleSwitch::ConnectSignals() {
    connect(combo_devices, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ConfigureSingleSwitch::OnDeviceChanged);
    connect(combo_controller_type, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ConfigureSingleSwitch::OnControllerTypeChanged);
    connect(combo_profiles, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ConfigureSingleSwitch::OnProfileChanged);

    connect(btn_save_profile, &QPushButton::clicked, this, &ConfigureSingleSwitch::OnSaveProfile);
    connect(btn_new_profile, &QPushButton::clicked, this, &ConfigureSingleSwitch::OnNewProfile);
    connect(btn_delete_profile, &QPushButton::clicked, this, &ConfigureSingleSwitch::OnDeleteProfile);

    connect(btn_restore_defaults, &QPushButton::clicked, this, &ConfigureSingleSwitch::RestoreDefaults);
    connect(btn_clear_all, &QPushButton::clicked, this, &ConfigureSingleSwitch::ClearMappings);

    // Helper for button connections
    auto bind_btn = [this](QPushButton* btn, std::size_t button_id) {
        connect(btn, &QPushButton::clicked, [this, btn, button_id] {
            HandleClick(
                btn, button_id,
                [this, button_id](const Common::ParamPackage& params) {
                    emulated_controller->SetButtonParam(button_id, params);
                },
                InputCommon::Polling::InputType::Button);
        });
    };

    // Face Buttons
    bind_btn(btn_face_a, Settings::NativeButton::A);
    bind_btn(btn_face_b, Settings::NativeButton::B);
    bind_btn(btn_face_x, Settings::NativeButton::X);
    bind_btn(btn_face_y, Settings::NativeButton::Y);

    // D-Pad
    bind_btn(btn_dpad_up, Settings::NativeButton::DUp);
    bind_btn(btn_dpad_down, Settings::NativeButton::DDown);
    bind_btn(btn_dpad_left, Settings::NativeButton::DLeft);
    bind_btn(btn_dpad_right, Settings::NativeButton::DRight);

    // Shoulders & Triggers
    bind_btn(btn_l, Settings::NativeButton::L);
    bind_btn(btn_r, Settings::NativeButton::R);
    bind_btn(btn_zl, Settings::NativeButton::ZL);
    bind_btn(btn_zr, Settings::NativeButton::ZR);
    bind_btn(btn_sl, Settings::NativeButton::SLLeft);
    bind_btn(btn_sr, Settings::NativeButton::SRLeft);

    // System Buttons
    bind_btn(btn_plus, Settings::NativeButton::Plus);
    bind_btn(btn_minus, Settings::NativeButton::Minus);
    bind_btn(btn_home, Settings::NativeButton::Home);
    bind_btn(btn_capture, Settings::NativeButton::Screenshot);

    // Stick Clicks
    bind_btn(btn_l3, Settings::NativeButton::LStick);
    bind_btn(btn_r3, Settings::NativeButton::RStick);

    // Stick Directions
    auto bind_stick_dir = [this](QPushButton* btn, std::size_t stick_id, const std::string& dir) {
        connect(btn, &QPushButton::clicked, [this, btn, stick_id, dir] {
            HandleClick(
                btn, stick_id,
                [this, stick_id, dir](const Common::ParamPackage& params) {
                    Common::ParamPackage stick_param = emulated_controller->GetStickParam(stick_id);
                    stick_param.Set(dir, params.Serialize());
                    emulated_controller->SetStickParam(stick_id, stick_param);
                },
                InputCommon::Polling::InputType::Button);
        });
    };

    bind_stick_dir(btn_lstick_up, Settings::NativeAnalog::LStick, "up");
    bind_stick_dir(btn_lstick_down, Settings::NativeAnalog::LStick, "down");
    bind_stick_dir(btn_lstick_left, Settings::NativeAnalog::LStick, "left");
    bind_stick_dir(btn_lstick_right, Settings::NativeAnalog::LStick, "right");

    bind_stick_dir(btn_rstick_up, Settings::NativeAnalog::RStick, "up");
    bind_stick_dir(btn_rstick_down, Settings::NativeAnalog::RStick, "down");
    bind_stick_dir(btn_rstick_left, Settings::NativeAnalog::RStick, "left");
    bind_stick_dir(btn_rstick_right, Settings::NativeAnalog::RStick, "right");

    connect(timeout_timer.get(), &QTimer::timeout, [this] { SetPollingResult({}, true); });
    connect(poll_timer.get(), &QTimer::timeout, [this] {
        Common::ParamPackage params = input_subsystem->GetNextInput();
        if (params.Serialize().empty()) {
            return;
        }
        if (IsInputAcceptable(params)) {
            SetPollingResult(params, false);
        }
    });
}

void ConfigureSingleSwitch::LoadConfiguration() {
    UpdateInputDeviceCombobox();
    UpdateInputProfilesCombobox();
    UpdateButtonTextLabels();
    UpdateControllerTypeVisibility();
}

void ConfigureSingleSwitch::UpdateInputDeviceCombobox() {
    combo_devices->clear();
    combo_devices->addItem(tr("Cualquiera / Múltiples"));
    combo_devices->addItem(tr("Sólo Teclado / Ratón"));

    const auto devices = input_subsystem->GetInputDevices();
    for (const auto& device : devices) {
        combo_devices->addItem(QString::fromStdString(device.Get("display", "Mando Desconocido")),
                               QString::fromStdString(device.Serialize()));
    }

    if (combo_devices->count() > 2) {
        combo_devices->setCurrentIndex(2);
    } else {
        combo_devices->setCurrentIndex(0);
    }
}

void ConfigureSingleSwitch::UpdateInputProfilesCombobox() {
    combo_profiles->clear();
    if (!profiles) return;

    const auto profile_names = profiles->GetInputProfileNames();
    for (const auto& name : profile_names) {
        combo_profiles->addItem(QString::fromStdString(name));
    }
}

void ConfigureSingleSwitch::UpdateControllerTypeVisibility() {
    const auto style = static_cast<Core::HID::NpadStyleIndex>(
        combo_controller_type->currentData().toInt());

    bool is_left_only = (style == Core::HID::NpadStyleIndex::JoyconLeft);
    bool is_right_only = (style == Core::HID::NpadStyleIndex::JoyconRight);

    left_stick_group->setVisible(!is_right_only);
    dpad_group->setVisible(!is_right_only);
    left_triggers_group->setVisible(!is_right_only);

    face_group->setVisible(!is_left_only);
    right_stick_group->setVisible(!is_left_only);
    right_triggers_group->setVisible(true);

    bool has_sl_sr = (style == Core::HID::NpadStyleIndex::JoyconDual ||
                      style == Core::HID::NpadStyleIndex::JoyconLeft ||
                      style == Core::HID::NpadStyleIndex::JoyconRight);
    joycon_sl_sr_group->setVisible(has_sl_sr);

    if (controller_frame) {
        controller_frame->ControllerUpdate(Core::HID::ControllerTriggerType::All);
    }
}

void ConfigureSingleSwitch::UpdateButtonTextLabels() {
    if (!emulated_controller) return;

    auto update_btn = [this](QPushButton* btn, std::size_t button_id) {
        const auto param = emulated_controller->GetButtonParam(button_id);
        btn->setText(ButtonToLabel(param, input_subsystem));
    };

    update_btn(btn_face_a, Settings::NativeButton::A);
    update_btn(btn_face_b, Settings::NativeButton::B);
    update_btn(btn_face_x, Settings::NativeButton::X);
    update_btn(btn_face_y, Settings::NativeButton::Y);

    update_btn(btn_dpad_up, Settings::NativeButton::DUp);
    update_btn(btn_dpad_down, Settings::NativeButton::DDown);
    update_btn(btn_dpad_left, Settings::NativeButton::DLeft);
    update_btn(btn_dpad_right, Settings::NativeButton::DRight);

    update_btn(btn_l, Settings::NativeButton::L);
    update_btn(btn_r, Settings::NativeButton::R);
    update_btn(btn_zl, Settings::NativeButton::ZL);
    update_btn(btn_zr, Settings::NativeButton::ZR);
    update_btn(btn_sl, Settings::NativeButton::SLLeft);
    update_btn(btn_sr, Settings::NativeButton::SRLeft);

    update_btn(btn_plus, Settings::NativeButton::Plus);
    update_btn(btn_minus, Settings::NativeButton::Minus);
    update_btn(btn_home, Settings::NativeButton::Home);
    update_btn(btn_capture, Settings::NativeButton::Screenshot);

    update_btn(btn_l3, Settings::NativeButton::LStick);
    update_btn(btn_r3, Settings::NativeButton::RStick);

    auto update_stick_dir = [this](QPushButton* btn, std::size_t stick_id, const std::string& dir) {
        const auto param = emulated_controller->GetStickParam(stick_id);
        Common::ParamPackage dir_param{param.Get(dir, "")};
        btn->setText(ButtonToLabel(dir_param, input_subsystem));
    };

    update_stick_dir(btn_lstick_up, Settings::NativeAnalog::LStick, "up");
    update_stick_dir(btn_lstick_down, Settings::NativeAnalog::LStick, "down");
    update_stick_dir(btn_lstick_left, Settings::NativeAnalog::LStick, "left");
    update_stick_dir(btn_lstick_right, Settings::NativeAnalog::LStick, "right");

    update_stick_dir(btn_rstick_up, Settings::NativeAnalog::RStick, "up");
    update_stick_dir(btn_rstick_down, Settings::NativeAnalog::RStick, "down");
    update_stick_dir(btn_rstick_left, Settings::NativeAnalog::RStick, "left");
    update_stick_dir(btn_rstick_right, Settings::NativeAnalog::RStick, "right");

    if (controller_frame) {
        controller_frame->update();
    }
}

void ConfigureSingleSwitch::HandleClick(QPushButton* button, std::size_t button_id,
                                        std::function<void(const Common::ParamPackage&)> new_input_setter,
                                        InputCommon::Polling::InputType type) {
    if (timeout_timer->isActive()) {
        return;
    }

    button->setText(tr("[Pulsa un botón...]"));
    button->setFocus();

    input_setter = std::move(new_input_setter);
    input_subsystem->BeginMapping(type);

    if (type == InputCommon::Polling::InputType::Button) {
        controller_frame->BeginMappingButton(button_id);
    } else if (type == InputCommon::Polling::InputType::Stick) {
        controller_frame->BeginMappingAnalog(button_id);
    }

    timeout_timer->start(4000);
    poll_timer->start(20);
}

void ConfigureSingleSwitch::SetPollingResult(const Common::ParamPackage& params, bool abort) {
    timeout_timer->stop();
    poll_timer->stop();
    input_subsystem->StopMapping();

    if (!abort && input_setter.has_value()) {
        (*input_setter)(params);
    }

    UpdateButtonTextLabels();
    controller_frame->EndMapping();
    input_setter = std::nullopt;
}

bool ConfigureSingleSwitch::IsInputAcceptable(const Common::ParamPackage& params) const {
    if (combo_devices->currentIndex() == 0) {
        return true;
    }
    if (combo_devices->currentIndex() == 1) {
        return params.Get("engine", "") == "keyboard" || params.Get("engine", "") == "mouse";
    }

    const std::string selected_serialized = combo_devices->currentData().toString().toStdString();
    Common::ParamPackage selected_params{selected_serialized};

    if (params.Get("engine", "") != selected_params.Get("engine", "")) {
        return false;
    }
    if (params.Has("guid") && selected_params.Has("guid")) {
        return params.Get("guid", "") == selected_params.Get("guid", "");
    }
    if (params.Has("port") && selected_params.Has("port")) {
        return params.Get("port", 0) == selected_params.Get("port", 0);
    }

    return true;
}

void ConfigureSingleSwitch::OnDeviceChanged(int index) {
    if (index <= 0) return;

    if (index == 1) {
        RestoreDefaults();
        return;
    }

    const std::string device_str = combo_devices->currentData().toString().toStdString();
    Common::ParamPackage device{device_str};

    const auto button_mappings = input_subsystem->GetButtonMappingForDevice(device);
    for (const auto& [btn_id, mapping_param] : button_mappings) {
        emulated_controller->SetButtonParam(btn_id, mapping_param);
    }

    const auto analog_mappings = input_subsystem->GetAnalogMappingForDevice(device);
    for (const auto& [analog_id, mapping_param] : analog_mappings) {
        emulated_controller->SetStickParam(analog_id, mapping_param);
    }

    UpdateButtonTextLabels();
}

void ConfigureSingleSwitch::OnControllerTypeChanged(int index) {
    if (!emulated_controller) return;

    const auto style = static_cast<Core::HID::NpadStyleIndex>(
        combo_controller_type->itemData(index).toInt());

    emulated_controller->SetNpadStyleIndex(style);
    UpdateControllerTypeVisibility();
}

void ConfigureSingleSwitch::OnProfileChanged(int index) {
    if (!profiles || index < 0) return;
    std::string profile_name = combo_profiles->itemText(index).toStdString();
    if (profiles->LoadProfile(profile_name, 0)) {
        emulated_controller->ReloadFromSettings();
        UpdateButtonTextLabels();
    }
}

void ConfigureSingleSwitch::OnNewProfile() {
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Nuevo Perfil Switch"),
                                         tr("Introduce el nombre del nuevo perfil:"),
                                         QLineEdit::Normal, tr("Mi Perfil Switch"), &ok);
    if (ok && !name.isEmpty() && profiles) {
        std::string profile_name = name.toStdString();
        if (profiles->CreateProfile(profile_name, 0)) {
            UpdateInputProfilesCombobox();
            combo_profiles->setCurrentText(name);
        }
    }
}

void ConfigureSingleSwitch::OnSaveProfile() {
    if (combo_profiles->currentText().isEmpty() || !profiles) {
        OnNewProfile();
        return;
    }
    std::string profile_name = combo_profiles->currentText().toStdString();
    ApplyConfiguration();
    profiles->SaveProfile(profile_name, 0);
    QMessageBox::information(this, tr("Guardado"), tr("El perfil '%1' se ha guardado exitosamente.").arg(combo_profiles->currentText()));
}

void ConfigureSingleSwitch::OnDeleteProfile() {
    if (combo_profiles->currentText().isEmpty() || !profiles) return;
    std::string profile_name = combo_profiles->currentText().toStdString();
    if (profiles->DeleteProfile(profile_name)) {
        UpdateInputProfilesCombobox();
    }
}

void ConfigureSingleSwitch::RestoreDefaults() {
    if (!emulated_controller) return;

    for (int i = 0; i < Settings::NativeButton::NumButtons; ++i) {
        const std::string default_param = InputCommon::GenerateKeyboardParam(QtConfig::default_buttons[i]);
        emulated_controller->SetButtonParam(i, Common::ParamPackage{default_param});
    }

    for (int i = 0; i < Settings::NativeAnalog::NumAnalogs; ++i) {
        const std::string default_param = InputCommon::GenerateAnalogParamFromKeys(
            QtConfig::default_analogs[i][0], QtConfig::default_analogs[i][1],
            QtConfig::default_analogs[i][2], QtConfig::default_analogs[i][3],
            QtConfig::default_stick_mod[i], 0.5f);
        emulated_controller->SetStickParam(i, Common::ParamPackage{default_param});
    }

    UpdateButtonTextLabels();
}

void ConfigureSingleSwitch::ClearMappings() {
    if (!emulated_controller) return;

    for (int i = 0; i < Settings::NativeButton::NumButtons; ++i) {
        emulated_controller->SetButtonParam(i, {});
    }
    for (int i = 0; i < Settings::NativeAnalog::NumAnalogs; ++i) {
        emulated_controller->SetStickParam(i, {});
    }

    UpdateButtonTextLabels();
}

void ConfigureSingleSwitch::ApplyConfiguration() {
    if (emulated_controller) {
        emulated_controller->SaveCurrentConfig();
    }
    QtConfig().SaveAllValues();
}

} // namespace GamepadMapper
