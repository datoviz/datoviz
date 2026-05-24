/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QGuiApplication>
#include <QSize>
#include <QString>
#include <QVulkanInstance>
#include <QWindow>

#include <stdint.h>
#include <vector>

#include "datoviz/app.h"
#include "datoviz/scene.h"



class DvzQtHostedWindow : public QWindow
{
  public:
    DvzQtHostedWindow(
        DvzApp* app, DvzFigure* figure, DvzPanel* panel, QVulkanInstance* instance,
        const QString& title, const QSize& initial_size);
    ~DvzQtHostedWindow() override;

    void schedule_frame();
    void request_scene_frame();
    void release_surface();
    void set_wheel_scale(float scale);

    DvzView* view() const;
    uint32_t request_count() const;

  protected:
    bool event(QEvent* event) override;
    void exposeEvent(QExposeEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    virtual void frame_rendered(int frame_status);

  private:
    DvzApp* _app = nullptr;
    DvzFigure* _figure = nullptr;
    DvzPanel* _panel = nullptr;
    QVulkanInstance* _instance = nullptr;
    DvzView* _view = nullptr;
    uint32_t _request_count = 0;
    bool _repaint_requested = true;
    float _wheel_scale = 1.0f;

    static uint32_t _positive_u32(int value);
    DvzWindowExternalSurfaceInfo _surface_info(VkSurfaceKHR surface) const;
    bool _ensure_initialized();
    void _emit_resize();
    void _emit_pointer(DvzPointerEventType type, QMouseEvent* event, DvzPointerButton button);
    void _emit_key(DvzKeyboardEventType type, QKeyEvent* event);
    void _render_once();
};



bool dvz_qt_instance_extensions(
    const QGuiApplication& app, std::vector<const char*>* extensions, const char* log_name);

