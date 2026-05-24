/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hosted_qt_smoke - host-owned Qt loop using Datoviz hosted rendering.
 *
 * Qt is used here as an external toolkit adapter. Datoviz does not create the Qt window and does
 * not enter dvz_app_run(); the host provides Vulkan instance extensions, Datoviz creates the
 * VkInstance, Qt creates VkSurfaceKHR from that instance, forwards input/resize events, and calls
 * dvz_view_render_once() from Qt's update path.
 *
 * Build:  just build
 * Run:    ./build/examples/qt/hosted_qt_smoke [frames]
 */

#include "hosted_qt_adapter.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QSize>
#include <QString>

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "datoviz/canvas/enums.h"



class HostedQtSmokeWindow : public DvzQtHostedWindow
{
  public:
    HostedQtSmokeWindow(
        DvzApp* app, DvzFigure* figure, DvzPanel* panel, QVulkanInstance* instance,
        uint32_t max_frames)
        : DvzQtHostedWindow(
              app, figure, panel, instance, QStringLiteral("hosted_qt_smoke"), QSize(800, 600)),
          _max_frames(max_frames)
    {}

    uint32_t frame_count() const
    {
        return _frame_count;
    }

  protected:
    void frame_rendered(int frame_status) override
    {
        if (frame_status == DVZ_CANVAS_FRAME_READY)
            _frame_count++;

        if (_max_frames != 0 && _frame_count >= _max_frames)
        {
            std::printf(
                "hosted_qt_smoke: rendered %u frame(s), %u request(s)\n", _frame_count,
                request_count());
            QCoreApplication::quit();
        }
        else if (_max_frames != 0)
        {
            schedule_frame();
        }
    }

  private:
    uint32_t _max_frames = 0;
    uint32_t _frame_count = 0;
};



static DvzScene* _make_scene(DvzFigure** out_figure, DvzPanel** out_panel)
{
    DvzScene* scene = dvz_scene();
    if (scene == nullptr)
        return nullptr;

    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    if (figure == nullptr)
    {
        dvz_scene_destroy(scene);
        return nullptr;
    }

    DvzPanelDesc panel_desc = {};
    panel_desc.x = 0.0f;
    panel_desc.y = 0.0f;
    panel_desc.width = 1.0f;
    panel_desc.height = 1.0f;
    DvzPanel* panel = dvz_panel(figure, panel_desc);
    DvzVisual* visual = panel != nullptr ? dvz_point(scene, 0) : nullptr;
    if (panel == nullptr || visual == nullptr)
    {
        dvz_scene_destroy(scene);
        return nullptr;
    }

    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f},
        { 0.0f,  0.5f, 0.0f},
    };
    uint8_t colors[3][4] = {
        {255,   0,   0, 255},
        {  0, 255,   0, 255},
        {  0,   0, 255, 255},
    };
    float sizes[3] = {24.0f, 24.0f, 24.0f};

    dvz_visual_set_data(visual, "position", positions, 3);
    dvz_visual_set_data(visual, "color", colors, 3);
    dvz_visual_set_data(visual, "diameter", sizes, 3);
    dvz_panel_add_visual(panel, visual, nullptr);
    dvz_panel_set_background_color(panel, 0.08f, 0.10f, 0.14f, 1.0f);

    if (out_figure != nullptr)
        *out_figure = figure;
    if (out_panel != nullptr)
        *out_panel = panel;
    return scene;
}



int main(int argc, char** argv)
{
    uint32_t max_frames = 120;
    if (argc > 1)
        max_frames = (uint32_t)std::strtoul(argv[1], nullptr, 10);

    QGuiApplication qt_app(argc, argv);

    std::vector<const char*> extensions;
    if (!dvz_qt_instance_extensions(qt_app, &extensions, "hosted_qt_smoke"))
        return 0;

    DvzFigure* figure = nullptr;
    DvzPanel* panel = nullptr;
    DvzScene* scene = _make_scene(&figure, &panel);
    if (scene == nullptr || figure == nullptr || panel == nullptr)
    {
        std::fprintf(stderr, "hosted_qt_smoke: failed to create scene\n");
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
        std::fprintf(stderr, "hosted_qt_smoke: skipped, Datoviz GPU context creation failed\n");
        dvz_scene_destroy(scene);
        return 0;
    }

    VkInstance instance = dvz_app_vk_instance(app);
    if (instance == VK_NULL_HANDLE)
    {
        std::fprintf(stderr, "hosted_qt_smoke: Datoviz returned no Vulkan instance\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    QVulkanInstance qt_instance;
    qt_instance.setVkInstance(instance);
    if (!qt_instance.create())
    {
        std::fprintf(
            stderr, "hosted_qt_smoke: Qt failed to adopt the Datoviz Vulkan instance (%d)\n",
            (int)qt_instance.errorCode());
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }

    HostedQtSmokeWindow* window =
        new HostedQtSmokeWindow(app, figure, panel, &qt_instance, max_frames);
    window->show();
    const int rc = qt_app.exec();
    window->release_surface();
    delete window;
    qt_instance.destroy();
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return rc;
}
