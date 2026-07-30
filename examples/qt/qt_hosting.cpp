/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* qt_hosting - This example hosts a Datoviz Vulkan scene in live Qt Widgets.
 *
 * Qt owns the event loop and widgets. Datoviz creates the Vulkan instance and renders into a
 * Qt-created VkSurfaceKHR through the hosted view contract.
 *
 * Build:  just build
 * Run:    ./build/examples/qt/qt_hosting
 * Smoke:  ./build/examples/qt/qt_hosting --smoke-ms 1000
 * Capture: ./build/examples/qt/qt_hosting --png
 */

#include "hosted_qt_adapter.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "datoviz/canvas.h"



namespace
{
constexpr int EXAMPLE_WINDOW_WIDTH = 1280;
constexpr int EXAMPLE_WINDOW_HEIGHT = 720;
constexpr int EXAMPLE_CONTROLS_WIDTH = 256;
constexpr int EXAMPLE_SCENE_WIDTH = EXAMPLE_WINDOW_WIDTH - EXAMPLE_CONTROLS_WIDTH;
} // namespace



static const uint32_t POINT_COUNT = 96;



static QString _capture_path()
{
    const QString directory = qEnvironmentVariable("DVZ_CAPTURE_DIR", QStringLiteral("."));
    const QString basename =
        qEnvironmentVariable("DVZ_CAPTURE_BASENAME", QStringLiteral("qt_hosting"));
    return QDir(directory).filePath(basename + QStringLiteral(".png"));
}



static bool _capture_window(DvzApp* app, DvzFigure* figure, QWidget* controls)
{
    const QString output_path = _capture_path();
    const QString scene_path = output_path + QStringLiteral(".scene.png");
    DvzView* capture_view =
        dvz_view_offscreen(app, figure, EXAMPLE_SCENE_WIDTH, EXAMPLE_WINDOW_HEIGHT);
    if (capture_view == nullptr ||
        dvz_view_render_once(capture_view) != DVZ_CANVAS_FRAME_READY ||
        dvz_view_capture_png(capture_view, scene_path.toUtf8().constData()) != 0)
        return false;

    QImage scene(scene_path);
    QFile::remove(scene_path);
    if (scene.isNull() || scene.size() != QSize(EXAMPLE_SCENE_WIDTH, EXAMPLE_WINDOW_HEIGHT))
        return false;

    controls->setFixedSize(EXAMPLE_CONTROLS_WIDTH, EXAMPLE_WINDOW_HEIGHT);
    if (controls->layout() != nullptr)
        controls->layout()->activate();
    QImage controls_image(
        EXAMPLE_CONTROLS_WIDTH, EXAMPLE_WINDOW_HEIGHT, QImage::Format_RGBA8888);
    controls_image.fill(controls->palette().window().color());
    QPainter controls_painter(&controls_image);
    controls->render(&controls_painter);
    controls_painter.end();

    QImage composed(EXAMPLE_WINDOW_WIDTH, EXAMPLE_WINDOW_HEIGHT, QImage::Format_RGBA8888);
    QPainter painter(&composed);
    painter.drawImage(0, 0, scene);
    painter.drawImage(EXAMPLE_SCENE_WIDTH, 0, controls_image);
    painter.end();
    return composed.save(output_path, "PNG");
}



struct SceneState
{
    DvzScene* scene = nullptr;
    DvzFigure* figure = nullptr;
    DvzPanel* panel = nullptr;
    DvzVisual* visual = nullptr;
    vec3 positions[POINT_COUNT] = {};
    DvzColor colors[POINT_COUNT] = {};
    float sizes[POINT_COUNT] = {};
    float point_size = 18.0f;
    float phase = 0.0f;
};



static void _fill_scene_data(SceneState* state)
{
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float t = (float)i / (float)POINT_COUNT;
        const float angle = t * 6.2831853f * 3.0f;
        const float radius = 0.18f + 0.62f * t;
        state->positions[i][0] = radius * std::cos(angle);
        state->positions[i][1] = radius * std::sin(angle);
        state->positions[i][2] = 0.0f;
        state->colors[i] = dvz_color_rgb(
            (uint8_t)(80 + (i * 97) % 176), (uint8_t)(80 + (i * 53) % 176),
            (uint8_t)(80 + (i * 29) % 176));
        state->sizes[i] = state->point_size;
    }
}



static int _apply_point_size(SceneState* state, float point_size)
{
    state->point_size = point_size;
    for (uint32_t i = 0; i < POINT_COUNT; i++)
        state->sizes[i] = point_size;
    return dvz_visual_set_data(state->visual, "diameter_px", state->sizes, POINT_COUNT);
}



static int _apply_palette(SceneState* state, int palette)
{
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const uint8_t a = (uint8_t)((i * 37) % 256);
        const uint8_t b = (uint8_t)((i * 67) % 256);
        if (palette == 0)
        {
            state->colors[i] = dvz_color_rgb(
                (uint8_t)(40 + (a % 180)), (uint8_t)(90 + (b % 130)),
                (uint8_t)(160 + ((a + b) % 80)));
        }
        else if (palette == 1)
        {
            state->colors[i] = dvz_color_rgb(
                (uint8_t)(180 + (a % 70)), (uint8_t)(70 + (b % 120)),
                (uint8_t)(80 + ((a + b) % 90)));
        }
        else
        {
            state->colors[i] = dvz_color_rgb(
                (uint8_t)(70 + (b % 110)), (uint8_t)(170 + (a % 70)),
                (uint8_t)(110 + ((a + b) % 120)));
        }
    }
    return dvz_visual_set_data(state->visual, "color", state->colors, POINT_COUNT);
}



static int _apply_animation_step(SceneState* state)
{
    state->phase += 0.035f;
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float t = (float)i / (float)POINT_COUNT;
        const float angle = t * 6.2831853f * 3.0f + state->phase;
        const float wave = 0.08f * std::sin(state->phase * 2.0f + t * 18.0f);
        const float radius = 0.18f + 0.62f * t + wave;
        state->positions[i][0] = radius * std::cos(angle);
        state->positions[i][1] = radius * std::sin(angle);
    }
    return dvz_visual_set_data(state->visual, "position", state->positions, POINT_COUNT);
}



static void _set_background(SceneState* state, int background)
{
    if (background == 0)
        dvz_panel_set_background_color(state->panel, dvz_color_from_unit(0.08f, 0.10f, 0.14f, 1.0f));
    else if (background == 1)
        dvz_panel_set_background_color(state->panel, dvz_color_from_unit(0.94f, 0.95f, 0.92f, 1.0f));
    else
        dvz_panel_set_background_color(state->panel, dvz_color_from_unit(0.03f, 0.11f, 0.10f, 1.0f));
}



static DvzScene* _make_scene(SceneState* state)
{
    DvzScene* scene = dvz_scene();
    if (scene == nullptr)
        return nullptr;

    DvzFigure* figure = dvz_figure(scene, EXAMPLE_WINDOW_WIDTH, EXAMPLE_WINDOW_HEIGHT, 0);
    DvzPanelDesc panel_desc = {};
    panel_desc.x = 0.0f;
    panel_desc.y = 0.0f;
    panel_desc.width = 1.0f;
    panel_desc.height = 1.0f;
    DvzPanel* panel = figure != nullptr ? dvz_panel(figure, &panel_desc) : nullptr;
    DvzVisual* visual = panel != nullptr ? dvz_point(scene, 0) : nullptr;
    if (figure == nullptr || panel == nullptr || visual == nullptr)
    {
        dvz_scene_destroy(scene);
        return nullptr;
    }

    state->scene = scene;
    state->figure = figure;
    state->panel = panel;
    state->visual = visual;
    _fill_scene_data(state);

    dvz_visual_set_data(visual, "position", state->positions, POINT_COUNT);
    dvz_visual_set_data(visual, "color", state->colors, POINT_COUNT);
    dvz_visual_set_data(visual, "diameter_px", state->sizes, POINT_COUNT);
    dvz_panel_add_visual(panel, visual, nullptr);
    _set_background(state, 0);
    return scene;
}



static QWidget* _controls_widget(SceneState* state, DvzQtHostedWindow* view_window, QTimer* timer)
{
    QWidget* controls = new QWidget();
    controls->setMinimumWidth(260);

    QVBoxLayout* layout = new QVBoxLayout(controls);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    QLabel* title = new QLabel(QStringLiteral("Scene controls"));
    QFont title_font = title->font();
    title_font.setBold(true);
    title->setFont(title_font);
    layout->addWidget(title);

    QLabel* size_label = new QLabel(QStringLiteral("Point size"));
    QSlider* size_slider = new QSlider(Qt::Horizontal);
    size_slider->setRange(4, 48);
    size_slider->setValue((int)state->point_size);
    layout->addWidget(size_label);
    layout->addWidget(size_slider);

    QLabel* wheel_label = new QLabel(QStringLiteral("Wheel sensitivity 100%"));
    QSlider* wheel_slider = new QSlider(Qt::Horizontal);
    wheel_slider->setRange(10, 200);
    wheel_slider->setValue(100);
    layout->addWidget(wheel_label);
    layout->addWidget(wheel_slider);

    QLabel* palette_label = new QLabel(QStringLiteral("Palette"));
    QComboBox* palette_combo = new QComboBox();
    palette_combo->addItem(QStringLiteral("Cool"));
    palette_combo->addItem(QStringLiteral("Warm"));
    palette_combo->addItem(QStringLiteral("Green"));
    layout->addWidget(palette_label);
    layout->addWidget(palette_combo);

    QLabel* background_label = new QLabel(QStringLiteral("Background"));
    QComboBox* background_combo = new QComboBox();
    background_combo->addItem(QStringLiteral("Dark"));
    background_combo->addItem(QStringLiteral("Light"));
    background_combo->addItem(QStringLiteral("Deep green"));
    layout->addWidget(background_label);
    layout->addWidget(background_combo);

    QCheckBox* animate_check = new QCheckBox(QStringLiteral("Animate"));
    QPushButton* reset_button = new QPushButton(QStringLiteral("Reset positions"));
    layout->addWidget(animate_check);
    layout->addWidget(reset_button);
    layout->addStretch(1);

    QObject::connect(size_slider, &QSlider::valueChanged, controls, [state, view_window](int value) {
        (void)_apply_point_size(state, (float)value);
        view_window->request_scene_frame();
    });
    QObject::connect(
        wheel_slider, &QSlider::valueChanged, controls,
        [view_window, wheel_label](int value) {
            view_window->set_wheel_scale((float)value / 100.0f);
            wheel_label->setText(QStringLiteral("Wheel sensitivity %1%").arg(value));
        });
    QObject::connect(
        palette_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), controls,
        [state, view_window](int index) {
            (void)_apply_palette(state, index);
            view_window->request_scene_frame();
        });
    QObject::connect(
        background_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), controls,
        [state, view_window](int index) {
            _set_background(state, index);
            view_window->request_scene_frame();
        });
    QObject::connect(animate_check, &QCheckBox::toggled, controls, [timer](bool checked) {
        if (checked)
            timer->start(16);
        else
            timer->stop();
    });
    QObject::connect(reset_button, &QPushButton::clicked, controls, [state, view_window]() {
        state->phase = 0.0f;
        _fill_scene_data(state);
        (void)dvz_visual_set_data(state->visual, "position", state->positions, POINT_COUNT);
        (void)dvz_visual_set_data(state->visual, "diameter_px", state->sizes, POINT_COUNT);
        view_window->request_scene_frame();
    });
    QObject::connect(timer, &QTimer::timeout, controls, [state, view_window]() {
        (void)_apply_animation_step(state);
        view_window->request_scene_frame();
    });

    return controls;
}



int main(int argc, char** argv)
{
    int smoke_ms = 0;
    bool capture_png = false;
    for (int i = 1; i < argc; i++)
    {
        const QString arg = QString::fromUtf8(argv[i]);
        if (arg == QStringLiteral("--png"))
            capture_png = true;
        else if (arg == QStringLiteral("--smoke-ms") && i + 1 < argc)
            smoke_ms = std::atoi(argv[i + 1]);
    }

    QApplication qt_app(argc, argv);

    std::vector<const char*> extensions;
    if (!dvz_qt_instance_extensions(qt_app, &extensions, "qt_hosting"))
        return 0;

    SceneState scene_state = {};
    DvzScene* scene = _make_scene(&scene_state);
    if (scene == nullptr)
    {
        std::fprintf(stderr, "qt_hosting: failed to create scene\n");
        return 1;
    }

    DvzAppConfig app_cfg = dvz_app_config();
    app_cfg.instance_extension_count = (uint32_t)extensions.size();
    app_cfg.instance_extensions = extensions.data();
    app_cfg.enable_canvas_extensions = true;
    app_cfg.enable_glfw_extensions = false;
    DvzApp* app = dvz_app_with_config(scene, &app_cfg);
    if (app == nullptr)
    {
        std::fprintf(stderr, "qt_hosting: skipped, Datoviz GPU context creation failed\n");
        dvz_scene_destroy(scene);
        return 0;
    }

    VkInstance instance = dvz_app_vk_instance(app);
    if (instance == VK_NULL_HANDLE)
    {
        std::fprintf(stderr, "qt_hosting: Datoviz returned no Vulkan instance\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    QVulkanInstance qt_instance;
    qt_instance.setVkInstance(instance);
    if (!qt_instance.create())
    {
        std::fprintf(
            stderr, "qt_hosting: Qt failed to adopt the Datoviz Vulkan instance (%d)\n",
            (int)qt_instance.errorCode());
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }

    DvzQtHostedWindow* view_window = new DvzQtHostedWindow(
        app, scene_state.figure, scene_state.panel, &qt_instance,
        QStringLiteral("qt_hosting_view"),
        QSize(EXAMPLE_WINDOW_WIDTH, EXAMPLE_WINDOW_HEIGHT));
    QWidget* view_container = QWidget::createWindowContainer(view_window);
    view_container->setMinimumSize(640, 480);
    view_container->setFocusPolicy(Qt::StrongFocus);

    QTimer timer;
    QWidget* controls = _controls_widget(&scene_state, view_window, &timer);

    QWidget main_widget;
    main_widget.setWindowTitle(QStringLiteral("Datoviz hosted Qt Widgets"));
    QHBoxLayout* root_layout = new QHBoxLayout(&main_widget);
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);
    root_layout->addWidget(view_container, 1);
    root_layout->addWidget(controls, 0);
    main_widget.resize(EXAMPLE_WINDOW_WIDTH, EXAMPLE_WINDOW_HEIGHT);
    main_widget.show();
    bool capture_failed = false;
    QObject::connect(&qt_app, &QCoreApplication::aboutToQuit, &qt_app, [&view_window]() {
        view_window->release_surface();
    });
    if (smoke_ms > 0)
        QTimer::singleShot(smoke_ms, &qt_app, &QCoreApplication::quit);
    if (capture_png)
    {
        QTimer::singleShot(1000, &qt_app, [app, &scene_state, controls, &capture_failed]() {
            if (!_capture_window(app, scene_state.figure, controls))
            {
                std::fprintf(stderr, "qt_hosting: failed to capture the Qt window\n");
                capture_failed = true;
            }
            QCoreApplication::quit();
        });
    }

    const int rc = qt_app.exec();
    timer.stop();
    view_window->release_surface();
    qt_instance.destroy();
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return capture_failed ? 1 : rc;
}
