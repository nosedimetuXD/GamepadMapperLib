// SPDX-License-Identifier: MIT
#include "configure_single_player.h"
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

ConfigureSinglePlayer::ConfigureSinglePlayer(Core::HID::HIDCore& hid_core_,
                                             InputCommon::InputSubsystem* input_subsystem_,
                                             InputProfiles* profiles_,
                                             ControllerLayoutStyle initial_style,
                                             QWidget* parent)
    : QWidget(parent), hid_core{hid_core_}, input_subsystem{input_subsystem_}, profiles{profiles_},
      current_style{initial_style},
      timeout_timer{std::make_unique<QTimer>()},
      poll_timer{std::make_unique<QTimer>()},
      pump_timer{std::make_unique<QTimer>()} {

    emulated_controller = hid_core.GetEmulatedController(Core::HID::NpadIdType::Player1);
    if (emulated_controller) {
        emulated_controller->EnableConfiguration();
        emulated_controller->Connect(true);
    }

    SetupLayout();
    ConnectSignals();
    LoadConfiguration();

    pump_timer->start(16); // 60 FPS event pump
}

ConfigureSinglePlayer::~ConfigureSinglePlayer() {
    if (emulated_controller) {
        emulated_controller->DisableConfiguration();
    }
}

void ConfigureSinglePlayer::SetupLayout() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(10);
    main_layout->setContentsMargins(10, 10, 10, 10);

    // ==========================================
    // Top Bar: Device, Style, Profile Selection
    // ==========================================
    auto* top_group = new QGroupBox(tr("Dispositivo y Perfiles"), this);
    auto* top_layout = new QGridLayout(top_group);

    check_connected = new QCheckBox(tr("Conectar Controlador"), this);
    check_connected->setChecked(true);
    top_layout->addWidget(check_connected, 0, 0);

    top_layout->addWidget(new QLabel(tr("Mando / Dispositivo:"), this), 0, 1);
    combo_devices = new QComboBox(this);
    top_layout->addWidget(combo_devices, 0, 2);

    top_layout->addWidget(new QLabel(tr("Estilo de Botones:"), this), 0, 3);
    combo_styles = new QComboBox(this);
    combo_styles->addItem(tr("Auto-Detectar"), static_cast<int>(ControllerLayoutStyle::AutoDetect));
    combo_styles->addItem(tr("Xbox (A/B/X/Y - LB/RB - LT/RT)"), static_cast<int>(ControllerLayoutStyle::Xbox));
    combo_styles->addItem(tr("PlayStation (✕/○/▢/△ - L1/R1 - L2/R2)"), static_cast<int>(ControllerLayoutStyle::PlayStation));
    combo_styles->addItem(tr("Nintendo (B/A/Y/X - L/R - ZL/ZR)"), static_cast<int>(ControllerLayoutStyle::Nintendo));
    combo_styles->addItem(tr("PC / Genérico"), static_cast<int>(ControllerLayoutStyle::GenericPC));
    combo_styles->setCurrentIndex(static_cast<int>(current_style));
    top_layout->addWidget(combo_styles, 0, 4);

    top_layout->addWidget(new QLabel(tr("Perfil:"), this), 1, 0);
    combo_profiles = new QComboBox(this);
    top_layout->addWidget(combo_profiles, 1, 1, 1, 2);

    auto* profile_btn_layout = new QHBoxLayout();
    btn_profile_save = new QPushButton(tr("Guardar"), this);
    btn_profile_new = new QPushButton(tr("Nuevo"), this);
    btn_profile_delete = new QPushButton(tr("Eliminar"), this);
    profile_btn_layout->addWidget(btn_profile_save);
    profile_btn_layout->addWidget(btn_profile_new);
    profile_btn_layout->addWidget(btn_profile_delete);
    top_layout->addLayout(profile_btn_layout, 1, 3, 1, 2);

    main_layout->addWidget(top_group);

    // ==========================================
    // Center Panels: Left Controls | Preview | Right Controls
    // ==========================================
    auto* center_layout = new QHBoxLayout();

    // --- Left Panel ---
    auto* left_panel = new QVBoxLayout();

    // Left Stick Group
    auto* left_stick_group = new QGroupBox(tr("Stick Izquierdo (Movimiento)"), this);
    auto* left_stick_grid = new QGridLayout(left_stick_group);
    btn_lstick_up = new QPushButton(this);
    btn_lstick_down = new QPushButton(this);
    btn_lstick_left = new QPushButton(this);
    btn_lstick_right = new QPushButton(this);
    btn_l3 = new QPushButton(this);
    lbl_l3 = new QLabel(tr("Click Stick (L3 / LS):"), this);

    left_stick_grid->addWidget(new QLabel(tr("Arriba:"), this), 0, 0);
    left_stick_grid->addWidget(btn_lstick_up, 0, 1);
    left_stick_grid->addWidget(new QLabel(tr("Abajo:"), this), 1, 0);
    left_stick_grid->addWidget(btn_lstick_down, 1, 1);
    left_stick_grid->addWidget(new QLabel(tr("Izquierda:"), this), 2, 0);
    left_stick_grid->addWidget(btn_lstick_left, 2, 1);
    left_stick_grid->addWidget(new QLabel(tr("Derecha:"), this), 3, 0);
    left_stick_grid->addWidget(btn_lstick_right, 3, 1);
    left_stick_grid->addWidget(lbl_l3, 4, 0);
    left_stick_grid->addWidget(btn_l3, 4, 1);
    left_panel->addWidget(left_stick_group);

    // D-Pad Group
    auto* dpad_group = new QGroupBox(tr("Cruceta Direccional (D-Pad)"), this);
    auto* dpad_grid = new QGridLayout(dpad_group);
    lbl_dpad_up = new QLabel(tr("Arriba:"), this);
    btn_dpad_up = new QPushButton(this);
    lbl_dpad_down = new QLabel(tr("Abajo:"), this);
    btn_dpad_down = new QPushButton(this);
    lbl_dpad_left = new QLabel(tr("Izquierda:"), this);
    btn_dpad_left = new QPushButton(this);
    lbl_dpad_right = new QLabel(tr("Derecha:"), this);
    btn_dpad_right = new QPushButton(this);

    dpad_grid->addWidget(lbl_dpad_up, 0, 0);
    dpad_grid->addWidget(btn_dpad_up, 0, 1);
    dpad_grid->addWidget(lbl_dpad_down, 1, 0);
    dpad_grid->addWidget(btn_dpad_down, 1, 1);
    dpad_grid->addWidget(lbl_dpad_left, 2, 0);
    dpad_grid->addWidget(btn_dpad_left, 2, 1);
    dpad_grid->addWidget(lbl_dpad_right, 3, 0);
    dpad_grid->addWidget(btn_dpad_right, 3, 1);
    left_panel->addWidget(dpad_group);

    // Left Triggers & Shoulders
    auto* left_triggers_group = new QGroupBox(tr("Gatillos Izquierdos"), this);
    auto* left_triggers_grid = new QGridLayout(left_triggers_group);
    lbl_l1 = new QLabel(tr("Bumper (LB / L1):"), this);
    btn_l1 = new QPushButton(this);
    lbl_l2 = new QLabel(tr("Gatillo (LT / L2):"), this);
    btn_l2 = new QPushButton(this);

    left_triggers_grid->addWidget(lbl_l1, 0, 0);
    left_triggers_grid->addWidget(btn_l1, 0, 1);
    left_triggers_grid->addWidget(lbl_l2, 1, 0);
    left_triggers_grid->addWidget(btn_l2, 1, 1);
    left_panel->addWidget(left_triggers_group);

    center_layout->addLayout(left_panel, 1);

    // --- Middle Panel: Visual Controller Preview ---
    auto* preview_group = new QGroupBox(tr("Vista Previa y Prueba en Tiempo Real"), this);
    auto* preview_layout = new QVBoxLayout(preview_group);
    controller_frame = new PlayerControlPreview(preview_group);
    controller_frame->setMinimumSize(320, 240);
    if (emulated_controller) {
        controller_frame->SetController(emulated_controller);
    }
    preview_layout->addWidget(controller_frame);
    center_layout->addWidget(preview_group, 2);

    // --- Right Panel ---
    auto* right_panel = new QVBoxLayout();

    // Face Buttons Group
    auto* face_group = new QGroupBox(tr("Botones Principales (Acción)"), this);
    auto* face_grid = new QGridLayout(face_group);
    lbl_face_a = new QLabel(tr("Botón Inferior (A / ✕):"), this);
    btn_face_a = new QPushButton(this);
    lbl_face_b = new QLabel(tr("Botón Derecho (B / ○):"), this);
    btn_face_b = new QPushButton(this);
    lbl_face_x = new QLabel(tr("Botón Izquierdo (X / ▢):"), this);
    btn_face_x = new QPushButton(this);
    lbl_face_y = new QLabel(tr("Botón Superior (Y / △):"), this);
    btn_face_y = new QPushButton(this);

    face_grid->addWidget(lbl_face_a, 0, 0);
    face_grid->addWidget(btn_face_a, 0, 1);
    face_grid->addWidget(lbl_face_b, 1, 0);
    face_grid->addWidget(btn_face_b, 1, 1);
    face_grid->addWidget(lbl_face_x, 2, 0);
    face_grid->addWidget(btn_face_x, 2, 1);
    face_grid->addWidget(lbl_face_y, 3, 0);
    face_grid->addWidget(btn_face_y, 3, 1);
    right_panel->addWidget(face_group);

    // Right Stick Group
    auto* right_stick_group = new QGroupBox(tr("Stick Derecho (Cámara / Habilidad)"), this);
    auto* right_stick_grid = new QGridLayout(right_stick_group);
    btn_rstick_up = new QPushButton(this);
    btn_rstick_down = new QPushButton(this);
    btn_rstick_left = new QPushButton(this);
    btn_rstick_right = new QPushButton(this);
    btn_r3 = new QPushButton(this);
    lbl_r3 = new QLabel(tr("Click Stick (R3 / RS):"), this);

    right_stick_grid->addWidget(new QLabel(tr("Arriba:"), this), 0, 0);
    right_stick_grid->addWidget(btn_rstick_up, 0, 1);
    right_stick_grid->addWidget(new QLabel(tr("Abajo:"), this), 1, 0);
    right_stick_grid->addWidget(btn_rstick_down, 1, 1);
    right_stick_grid->addWidget(new QLabel(tr("Izquierda:"), this), 2, 0);
    right_stick_grid->addWidget(btn_rstick_left, 2, 1);
    right_stick_grid->addWidget(new QLabel(tr("Derecha:"), this), 3, 0);
    right_stick_grid->addWidget(btn_rstick_right, 3, 1);
    right_stick_grid->addWidget(lbl_r3, 4, 0);
    right_stick_grid->addWidget(btn_r3, 4, 1);
    right_panel->addWidget(right_stick_group);

    // Right Triggers & Shoulders + Menu
    auto* right_triggers_group = new QGroupBox(tr("Gatillos Derechos y Menú"), this);
    auto* right_triggers_grid = new QGridLayout(right_triggers_group);
    lbl_r1 = new QLabel(tr("Bumper (RB / R1):"), this);
    btn_r1 = new QPushButton(this);
    lbl_r2 = new QLabel(tr("Gatillo (RT / R2):"), this);
    btn_r2 = new QPushButton(this);
    lbl_start = new QLabel(tr("Pausa / Menu (Start):"), this);
    btn_start = new QPushButton(this);
    lbl_select = new QLabel(tr("Atrás / Vista (Back):"), this);
    btn_select = new QPushButton(this);

    right_triggers_grid->addWidget(lbl_r1, 0, 0);
    right_triggers_grid->addWidget(btn_r1, 0, 1);
    right_triggers_grid->addWidget(lbl_r2, 1, 0);
    right_triggers_grid->addWidget(btn_r2, 1, 1);
    right_triggers_grid->addWidget(lbl_start, 2, 0);
    right_triggers_grid->addWidget(btn_start, 2, 1);
    right_triggers_grid->addWidget(lbl_select, 3, 0);
    right_triggers_grid->addWidget(btn_select, 3, 1);
    right_panel->addWidget(right_triggers_group);

    center_layout->addLayout(right_panel, 1);
    main_layout->addLayout(center_layout);

    // ==========================================
    // Bottom Controls: Restore Defaults, Clear
    // ==========================================
    auto* bottom_layout = new QHBoxLayout();
    btn_restore_defaults = new QPushButton(tr("Restablecer por Defecto"), this);
    btn_clear_all = new QPushButton(tr("Limpiar Mapeos"), this);
    bottom_layout->addWidget(btn_restore_defaults);
    bottom_layout->addWidget(btn_clear_all);
    bottom_layout->addStretch();
    main_layout->addLayout(bottom_layout);
}

void ConfigureSinglePlayer::ConnectSignals() {
    timeout_timer->setSingleShot(true);
    connect(timeout_timer.get(), &QTimer::timeout, [this] { SetPollingResult({}, true); });

    connect(poll_timer.get(), &QTimer::timeout, [this] {
        input_subsystem->PumpEvents();
        const auto& params = input_subsystem->GetNextInput();
        if (params.Has("engine") && IsInputAcceptable(params)) {
            SetPollingResult(params, false);
            return;
        }
    });

    connect(pump_timer.get(), &QTimer::timeout, [this] {
        input_subsystem->PumpEvents();
    });

    connect(check_connected, &QCheckBox::toggled, this, [this](bool checked) {
        if (emulated_controller) {
            emulated_controller->Connect(checked);
        }
    });
    connect(combo_devices, qOverload<int>(&QComboBox::activated), this, &ConfigureSinglePlayer::OnDeviceChanged);
    connect(combo_styles, qOverload<int>(&QComboBox::currentIndexChanged), this, &ConfigureSinglePlayer::OnStyleChanged);
    connect(btn_profile_new, &QPushButton::clicked, this, &ConfigureSinglePlayer::OnNewProfile);
    connect(btn_profile_save, &QPushButton::clicked, this, &ConfigureSinglePlayer::OnSaveProfile);
    connect(btn_profile_delete, &QPushButton::clicked, this, &ConfigureSinglePlayer::OnDeleteProfile);
    connect(combo_profiles, qOverload<int>(&QComboBox::activated), this, &ConfigureSinglePlayer::OnLoadProfile);
    connect(btn_restore_defaults, &QPushButton::clicked, this, &ConfigureSinglePlayer::RestoreDefaults);
    connect(btn_clear_all, &QPushButton::clicked, this, &ConfigureSinglePlayer::ClearAll);

    // Bind Button Click Slots
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

    // Face buttons: Switch A (0), B (1), X (2), Y (3)
    bind_btn(btn_face_a, Settings::NativeButton::A);
    bind_btn(btn_face_b, Settings::NativeButton::B);
    bind_btn(btn_face_x, Settings::NativeButton::X);
    bind_btn(btn_face_y, Settings::NativeButton::Y);

    // Shoulders & Triggers: L (6), R (7), ZL (8), ZR (9)
    bind_btn(btn_l1, Settings::NativeButton::L);
    bind_btn(btn_r1, Settings::NativeButton::R);
    bind_btn(btn_l2, Settings::NativeButton::ZL);
    bind_btn(btn_r2, Settings::NativeButton::ZR);

    // Sticks clicks: LStick (4), RStick (5)
    bind_btn(btn_l3, Settings::NativeButton::LStick);
    bind_btn(btn_r3, Settings::NativeButton::RStick);

    // D-Pad: Left (12), Up (13), Right (14), Down (15)
    bind_btn(btn_dpad_up, Settings::NativeButton::DUp);
    bind_btn(btn_dpad_down, Settings::NativeButton::DDown);
    bind_btn(btn_dpad_left, Settings::NativeButton::DLeft);
    bind_btn(btn_dpad_right, Settings::NativeButton::DRight);

    // Menu: Plus (10), Minus (11)
    bind_btn(btn_start, Settings::NativeButton::Plus);
    bind_btn(btn_select, Settings::NativeButton::Minus);

    // Stick directions (Analog mapping)
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
}

void ConfigureSinglePlayer::LoadConfiguration() {
    if (emulated_controller) {
        emulated_controller->ReloadFromSettings();
        if (check_connected) {
            check_connected->setChecked(emulated_controller->IsConnected(true));
        }
    }
    UpdateInputDeviceCombobox();
    UpdateInputProfilesCombobox();
    UpdateLabelsForCurrentStyle();
    UpdateButtonTextLabels();
}

void ConfigureSinglePlayer::UpdateInputDeviceCombobox() {
    combo_devices->blockSignals(true);
    combo_devices->clear();
    combo_devices->addItem(tr("Cualquiera / Múltiples"), QString{});
    combo_devices->addItem(tr("Sólo Teclado / Ratón"), QString{});

    const auto devices = input_subsystem->GetInputDevices();
    for (const auto& device : devices) {
        combo_devices->addItem(QString::fromStdString(device.Get("display", "Mando Desconocido")),
                               QString::fromStdString(device.Serialize()));
    }

    int selected_idx = 0;
    const auto mapped = emulated_controller ? emulated_controller->GetMappedDevices() : std::vector<Common::ParamPackage>{};
    if (!mapped.empty()) {
        const auto first_engine = mapped[0].Get("engine", "");
        const auto first_guid = mapped[0].Get("guid", "");
        const auto first_port = mapped[0].Get("port", 0);

        if (first_engine == "keyboard" || first_engine == "mouse") {
            selected_idx = 1;
        } else {
            for (int i = 2; i < combo_devices->count(); ++i) {
                Common::ParamPackage dev_param{combo_devices->itemData(i).toString().toStdString()};
                if (dev_param.Get("engine", "") == first_engine &&
                    (dev_param.Get("guid", "") == first_guid || dev_param.Get("guid2", "") == first_guid) &&
                    dev_param.Get("port", 0) == first_port) {
                    selected_idx = i;
                    break;
                }
            }
            if (selected_idx == 0 && combo_devices->count() > 2) {
                selected_idx = 2;
            }
        }
    } else if (combo_devices->count() > 2) {
        selected_idx = 2; // Seleccionar el primer mando físico conectado
    }

    combo_devices->setCurrentIndex(selected_idx);
    combo_devices->blockSignals(false);
}

void ConfigureSinglePlayer::UpdateInputProfilesCombobox() {
    combo_profiles->blockSignals(true);
    combo_profiles->clear();
    if (profiles) {
        const auto profile_names = profiles->GetInputProfileNames();
        const auto& current_profile = Settings::values.players.GetValue()[0].profile_name;
        int active_idx = -1;
        for (std::size_t i = 0; i < profile_names.size(); ++i) {
            combo_profiles->addItem(QString::fromStdString(profile_names[i]));
            if (profile_names[i] == current_profile) {
                active_idx = static_cast<int>(i);
            }
        }
        if (active_idx >= 0) {
            combo_profiles->setCurrentIndex(active_idx);
        }
    }
    combo_profiles->blockSignals(false);
}

void ConfigureSinglePlayer::SetLayoutStyle(ControllerLayoutStyle style) {
    current_style = style;
    combo_styles->setCurrentIndex(static_cast<int>(style));
    UpdateLabelsForCurrentStyle();
}

void ConfigureSinglePlayer::UpdateLabelsForCurrentStyle() {
    ControllerLayoutStyle active_style = current_style;

    if (active_style == ControllerLayoutStyle::AutoDetect) {
        QString dev_text = combo_devices->currentText().toLower();
        if (dev_text.contains("ps") || dev_text.contains("playstation") || dev_text.contains("dual") || dev_text.contains("sony")) {
            active_style = ControllerLayoutStyle::PlayStation;
        } else if (dev_text.contains("switch") || dev_text.contains("pro controller") || dev_text.contains("joycon") || dev_text.contains("nintendo")) {
            active_style = ControllerLayoutStyle::Nintendo;
        } else {
            active_style = ControllerLayoutStyle::Xbox; // Default PC / Xbox
        }
    }

    if (controller_frame) {
        if (active_style == ControllerLayoutStyle::PlayStation) {
            controller_frame->SetVisualLayoutStyle(PlayerControlPreview::VisualLayoutStyle::PlayStation);
        } else if (active_style == ControllerLayoutStyle::Xbox || active_style == ControllerLayoutStyle::GenericPC) {
            controller_frame->SetVisualLayoutStyle(PlayerControlPreview::VisualLayoutStyle::Xbox);
        } else {
            controller_frame->SetVisualLayoutStyle(PlayerControlPreview::VisualLayoutStyle::Nintendo);
        }
    }

    switch (active_style) {
    case ControllerLayoutStyle::PlayStation:
        lbl_face_a->setText(tr("Botón Inferior ( ✕ Cruz ):"));
        lbl_face_b->setText(tr("Botón Derecho ( ○ Círculo ):"));
        lbl_face_x->setText(tr("Botón Izquierdo ( ▢ Cuadrado ):"));
        lbl_face_y->setText(tr("Botón Superior ( △ Triángulo ):"));
        lbl_l1->setText(tr("Bumper Izquierdo ( L1 ):"));
        lbl_r1->setText(tr("Bumper Derecho ( R1 ):"));
        lbl_l2->setText(tr("Gatillo Izquierdo ( L2 ):"));
        lbl_r2->setText(tr("Gatillo Derecho ( R2 ):"));
        lbl_start->setText(tr("Opciones ( Options ):"));
        lbl_select->setText(tr("Compartir ( Share / Create ):"));
        lbl_l3->setText(tr("Click Stick Izquierdo ( L3 ):"));
        lbl_r3->setText(tr("Click Stick Derecho ( R3 ):"));
        break;

    case ControllerLayoutStyle::Nintendo:
        lbl_face_a->setText(tr("Botón Derecho ( A ):"));
        lbl_face_b->setText(tr("Botón Inferior ( B ):"));
        lbl_face_x->setText(tr("Botón Superior ( X ):"));
        lbl_face_y->setText(tr("Botón Izquierdo ( Y ):"));
        lbl_l1->setText(tr("Botón L ( L ):"));
        lbl_r1->setText(tr("Botón R ( R ):"));
        lbl_l2->setText(tr("Gatillo ZL ( ZL ):"));
        lbl_r2->setText(tr("Gatillo ZR ( ZR ):"));
        lbl_start->setText(tr("Botón Más ( + ):"));
        lbl_select->setText(tr("Botón Menos ( - ):"));
        lbl_l3->setText(tr("Click Stick L:"));
        lbl_r3->setText(tr("Click Stick R:"));
        break;

    case ControllerLayoutStyle::Xbox:
    case ControllerLayoutStyle::GenericPC:
    default:
        lbl_face_a->setText(tr("Botón Inferior ( A ):"));
        lbl_face_b->setText(tr("Botón Derecho ( B ):"));
        lbl_face_x->setText(tr("Botón Izquierdo ( X ):"));
        lbl_face_y->setText(tr("Botón Superior ( Y ):"));
        lbl_l1->setText(tr("Bumper Izquierdo ( LB ):"));
        lbl_r1->setText(tr("Bumper Derecho ( RB ):"));
        lbl_l2->setText(tr("Gatillo Izquierdo ( LT ):"));
        lbl_r2->setText(tr("Gatillo Derecho ( RT ):"));
        lbl_start->setText(tr("Menú / Inicio ( Menu / Start ):"));
        lbl_select->setText(tr("Vista / Atrás ( View / Back ):"));
        lbl_l3->setText(tr("Click Stick Izq ( LS ):"));
        lbl_r3->setText(tr("Click Stick Der ( RS ):"));
        break;
    }
}

void ConfigureSinglePlayer::UpdateButtonTextLabels() {
    if (!emulated_controller) return;

    auto update_btn = [this](QPushButton* btn, std::size_t button_id) {
        const auto param = emulated_controller->GetButtonParam(button_id);
        btn->setText(ButtonToLabel(param, input_subsystem));
    };

    update_btn(btn_face_a, Settings::NativeButton::A);
    update_btn(btn_face_b, Settings::NativeButton::B);
    update_btn(btn_face_x, Settings::NativeButton::X);
    update_btn(btn_face_y, Settings::NativeButton::Y);

    update_btn(btn_l1, Settings::NativeButton::L);
    update_btn(btn_r1, Settings::NativeButton::R);
    update_btn(btn_l2, Settings::NativeButton::ZL);
    update_btn(btn_r2, Settings::NativeButton::ZR);

    update_btn(btn_l3, Settings::NativeButton::LStick);
    update_btn(btn_r3, Settings::NativeButton::RStick);

    update_btn(btn_dpad_up, Settings::NativeButton::DUp);
    update_btn(btn_dpad_down, Settings::NativeButton::DDown);
    update_btn(btn_dpad_left, Settings::NativeButton::DLeft);
    update_btn(btn_dpad_right, Settings::NativeButton::DRight);

    update_btn(btn_start, Settings::NativeButton::Plus);
    update_btn(btn_select, Settings::NativeButton::Minus);

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
}

void ConfigureSinglePlayer::HandleClick(QPushButton* button, std::size_t button_id,
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

    timeout_timer->start(4000); // 4 seconds timeout
    poll_timer->start(20);      // Poll at 50Hz
}

void ConfigureSinglePlayer::SetPollingResult(const Common::ParamPackage& params, bool abort) {
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

bool ConfigureSinglePlayer::IsInputAcceptable(const Common::ParamPackage& params) const {
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

void ConfigureSinglePlayer::OnDeviceChanged(int index) {
    if (index <= 0) return;

    if (index == 1) { // Solo Teclado
        RestoreDefaults();
        return;
    }

    const std::string device_str = combo_devices->currentData().toString().toStdString();
    Common::ParamPackage device{device_str};

    // Auto-mapeo nativo de los botones del mando conectado
    const auto button_mappings = input_subsystem->GetButtonMappingForDevice(device);
    for (const auto& [btn_id, mapping_param] : button_mappings) {
        emulated_controller->SetButtonParam(btn_id, mapping_param);
    }

    const auto analog_mappings = input_subsystem->GetAnalogMappingForDevice(device);
    for (const auto& [analog_id, mapping_param] : analog_mappings) {
        emulated_controller->SetStickParam(analog_id, mapping_param);
    }

    UpdateLabelsForCurrentStyle();
    UpdateButtonTextLabels();
}

void ConfigureSinglePlayer::OnStyleChanged(int index) {
    current_style = static_cast<ControllerLayoutStyle>(index);
    UpdateLabelsForCurrentStyle();
}

void ConfigureSinglePlayer::OnNewProfile() {
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Nuevo Perfil"),
                                         tr("Introduce el nombre del nuevo perfil:"),
                                         QLineEdit::Normal, tr("Mi Perfil"), &ok);
    if (ok && !name.isEmpty() && profiles) {
        std::string profile_name = name.toStdString();
        if (profiles->CreateProfile(profile_name, 0)) {
            UpdateInputProfilesCombobox();
            combo_profiles->setCurrentText(name);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Ya existe un perfil con ese nombre o es inválido."));
        }
    }
}

void ConfigureSinglePlayer::OnSaveProfile() {
    if (combo_profiles->currentText().isEmpty() || !profiles) {
        OnNewProfile();
        return;
    }
    std::string profile_name = combo_profiles->currentText().toStdString();
    profiles->SaveProfile(profile_name, 0);
    QMessageBox::information(this, tr("Guardado"), tr("El perfil '%1' se ha guardado correctamente.").arg(combo_profiles->currentText()));
}

void ConfigureSinglePlayer::OnDeleteProfile() {
    if (combo_profiles->currentText().isEmpty() || !profiles) return;
    std::string profile_name = combo_profiles->currentText().toStdString();
    if (profiles->DeleteProfile(profile_name)) {
        UpdateInputProfilesCombobox();
    }
}

void ConfigureSinglePlayer::OnLoadProfile(int index) {
    if (!profiles || index < 0) return;
    std::string profile_name = combo_profiles->itemText(index).toStdString();
    if (profiles->LoadProfile(profile_name, 0)) {
        emulated_controller->ReloadFromSettings();
        UpdateButtonTextLabels();
    }
}

void ConfigureSinglePlayer::RestoreDefaults() {
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

void ConfigureSinglePlayer::ClearAll() {
    if (!emulated_controller) return;

    for (int i = 0; i < Settings::NativeButton::NumButtons; ++i) {
        emulated_controller->SetButtonParam(i, {});
    }
    for (int i = 0; i < Settings::NativeAnalog::NumAnalogs; ++i) {
        emulated_controller->SetStickParam(i, {});
    }

    UpdateButtonTextLabels();
}

void ConfigureSinglePlayer::ApplyConfiguration() {
    if (emulated_controller) {
        if (check_connected) {
            emulated_controller->Connect(check_connected->isChecked());
        }
        emulated_controller->SaveCurrentConfig();
    }
    if (combo_profiles && !combo_profiles->currentText().isEmpty()) {
        Settings::values.players.GetValue()[0].profile_name = combo_profiles->currentText().toStdString();
    }
    QtConfig config;
    config.SaveAllValues();
}

} // namespace GamepadMapper
