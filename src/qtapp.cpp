/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Qt app                                                                                       */
/*************************************************************************************************/

#include "qtapp.hpp"
#include "canvas.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "host.h"
#include "recorder.h"
#include "render_utils.h"
#include "renderer.h"
#include "vklite.h"
#include "workspace.h"


#if HAS_QT

#include <QFile>
#include <QGuiApplication>
#include <QVersionNumber>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <QVulkanWindow>



/*************************************************************************************************/
/*  Classes                                                                                      */
/*************************************************************************************************/

// Vertex structure
struct Vertex
{
    QVector2D position;
    QVector3D color;
};



class VulkanRenderer : public QVulkanWindowRenderer
{
public:
    VulkanRenderer(DvzQtWindow* window, DvzQtApp* app);

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void startNextFrame() override;

    void setCanvas(DvzCanvas* canvas);

private:
    DvzQtWindow* m_window;
    QVulkanDeviceFunctions* m_devFuncs;
    DvzQtApp* m_app;
    DvzCanvas* m_canvas;
    QSize m_size;
    int m_swapchain_image_count;
};



class DvzQtWindow : public QVulkanWindow
{
public:
    DvzQtWindow(DvzQtApp* app);
    QVulkanWindowRenderer* createRenderer() override;
    void setId(DvzId);
    DvzId m_id;

private:
    DvzQtApp* m_app;
};



DvzQtWindow::DvzQtWindow(DvzQtApp* app) : m_app(app) {}

QVulkanWindowRenderer* DvzQtWindow::createRenderer() { return new VulkanRenderer(this, m_app); }

void DvzQtWindow::setId(DvzId id) { m_id = id; }



VulkanRenderer::VulkanRenderer(DvzQtWindow* window, DvzQtApp* app) : m_window(window), m_app(app)
{
}

void VulkanRenderer::initResources()
{
    QVulkanInstance* inst = m_window->vulkanInstance();
    m_devFuncs = inst->deviceFunctions(m_window->device());

    DvzBatch* batch = dvz_qt_batch(m_app);
    dvz_qt_submit(m_app, batch);
}

void VulkanRenderer::initSwapChainResources()
{
    m_size = m_window->swapChainImageSize();
    m_swapchain_image_count = m_window->swapChainImageCount();
}

void VulkanRenderer::releaseSwapChainResources() {}

void VulkanRenderer::releaseResources() {}

void VulkanRenderer::startNextFrame()
{
    ANN(m_window);

    DvzRenderer* rd = m_app->rd;
    ANN(rd);

    DvzGpu* gpu = rd->gpu;
    ANN(gpu);

    // Recover the canvas associated to the window via the id (the canvas and the window share the
    // same id).
    DvzCanvas* canvas = dvz_renderer_canvas(rd, m_window->m_id);
    ANN(canvas);

    // Swapchain image index.
    int img_idx = m_window->currentSwapChainImageIndex();

    // Set the current command buffer in the canvas.
    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    canvas->cmds.count = (uint32_t)m_swapchain_image_count;
    canvas->cmds.cmds[img_idx] = cmdBuf;

    // Begin render pass with a clear blue color
    VkClearValue clearColor = {};
    clearColor.color.float32[0] = 0.0f;
    clearColor.color.float32[1] = 0.0f;
    clearColor.color.float32[2] = 1.0f;
    clearColor.color.float32[3] = 1.0f;

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_window->defaultRenderPass();
    renderPassInfo.framebuffer = m_window->currentFramebuffer();
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {
        static_cast<uint32_t>(m_size.width()), static_cast<uint32_t>(m_size.height())};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    m_devFuncs->vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // HACK: this should be set outside of the main loop, but the recorder is being created lazily
    // for some reason. Will need to refactor the logic in the renderer so that the canvas recorder
    // is created upon canvas creation.

    // NOTE: we deactivate the recorder callbacks for begin/end as
    // they are dealt by the Qt Vulkan renderer directly.
    if (canvas->recorder == NULL)
    {
        canvas->recorder = dvz_recorder(0);
        ANN(canvas->recorder);
    }
    dvz_recorder_register(canvas->recorder, DVZ_RECORDER_BEGIN, NULL, NULL);
    dvz_recorder_register(canvas->recorder, DVZ_RECORDER_END, NULL, NULL);


    // TODO: only call this if dirty on the current img idx.
    // Update the recorder, skipping begin/end renderpass which are dealt with manually here (we
    // registered blank recorder callbacks for the begin/end record types).
    ANN(canvas->recorder);
    dvz_recorder_set(canvas->recorder, rd, &canvas->cmds, (uint32_t)img_idx);

    m_devFuncs->vkCmdEndRenderPass(cmdBuf);
    m_window->frameReady();
    m_window->requestUpdate();
}

void VulkanRenderer::setCanvas(DvzCanvas* canvas) { m_canvas = canvas; }



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



DvzQtApp* dvz_qt_app(QApplication* qapp, int flags)
{
    DvzQtApp* app = (DvzQtApp*)calloc(1, sizeof(DvzQtApp));
    ANN(app);

    app->qapp = qapp;

    // Create the Vulkan instance with Qt.
    QVulkanInstance* inst = new QVulkanInstance();
    // Vulkan API version.
    inst->setApiVersion(QVersionNumber(1, 2, 0));
    if (!inst->create())
        qFatal("Failed to create Vulkan instance");
    app->inst = inst;

    // Now, we create the host.
    app->host = dvz_host(DVZ_BACKEND_QT);
    ANN(app->host);

    // NOTE: we manually pass the Vulkan instance to the host before finishing creating the host.
    app->host->instance = inst->vkInstance();
    app->host->no_instance_destroy = true;
    dvz_host_create(app->host);
    log_info("host created");

// Start creating a GPU, but with QT backend, do NOT call dvz_gpu_create() just yet
// as we want to create a mock window/surface first.
TODO:
    qt
        // app->gpu = make_gpu(app->host);
        ANN(app->gpu);


    // HACK: Create a mock window to get a surface handle and call
    // dvz_gpu_create(app->gpu, surface) afterwards.
    QWindow window;
    window.setSurfaceType(QSurface::VulkanSurface);
    window.setVulkanInstance(inst);
    window.create();
    VkSurfaceKHR surface = inst->surfaceForWindow(&window);
    dvz_gpu_create(app->gpu, surface);
    window.destroy();


    // Create a renderer without workspace (which would handle boards and canvases).
    app->rd = dvz_renderer(app->gpu, flags);
    ANN(app->rd);

    // Create a batch.
    app->batch = dvz_batch();
    ANN(app->batch);

    return app;
}



DvzQtWindow* dvz_qt_window(DvzQtApp* app)
{
    ANN(app);
    DvzQtWindow* window = new DvzQtWindow(app);
    window->setVulkanInstance(app->inst);
    return (DvzQtWindow*)window;
}



static void _create_canvas(DvzQtApp* app, DvzRequest req)
{
    ANN(app);

    DvzRenderer* rd = app->rd;
    ANN(rd);

    DvzQtWindow* window = dvz_qt_window(app);
    ANN(window);
    window->show();

    // Register the window id.
    window->setId(req.id);

    // Recover the created canvas.
    DvzCanvas* canvas = dvz_renderer_canvas(rd, req.id);
}

static void _delete_canvas(DvzQtApp* app, DvzId id)
{
    ANN(app);

    DvzRenderer* rd = app->rd;
    ANN(rd);

    DvzGpu* gpu = rd->gpu;
    ANN(gpu);

    // Wait for all GPU processing to stop.
    dvz_gpu_wait(gpu);

    // Start canvas destruction.
    DvzCanvas* canvas = dvz_renderer_canvas(rd, id);
    ANN(canvas);

    // TODO

    // // Then, destroy the canvas.
    // dvz_canvas_destroy(canvas);

    // Destroy the canvas recorder.
    if (canvas->recorder != NULL)
        dvz_recorder_destroy(canvas->recorder);
}

static void _canvas_request(DvzQtApp* app, DvzRequest req)
{
    ANN(app);
    switch (req.action)
    {
    case DVZ_REQUEST_ACTION_CREATE:;
        log_debug("process canvas creation request");
        _create_canvas(app, req);
        break;
    case DVZ_REQUEST_ACTION_DELETE:;
        log_debug("process canvas deletion request");
        _delete_canvas(app, req.id);
        break;
    default:
        break;
    }
}



void dvz_qt_submit(DvzQtApp* app, DvzBatch* batch)
{
    ANN(app);
    ANN(batch);

    DvzRenderer* rd = app->rd;
    ANN(rd);

    uint32_t count = dvz_batch_size(batch);
    DvzRequest* requests = dvz_batch_requests(batch);
    ANN(requests);

    for (uint32_t i = 0; i < count; i++)
    {
        // Process each request immediately in the renderer.
        dvz_renderer_request(rd, requests[i]);

        // CANVAS requests need special care.
        if (requests[i].type == DVZ_REQUEST_OBJECT_CANVAS)
        {
            _canvas_request(app, requests[i]);
        }
    }

    dvz_batch_clear(batch);
}



DvzBatch* dvz_qt_batch(DvzQtApp* app)
{
    ANN(app);
    return app->batch;
}



void dvz_qt_app_destroy(DvzQtApp* app)
{
    // Destroy the batch.
    dvz_batch_destroy(app->batch);

    // Destroy and free the renderpass.
    // dvz_renderpass_destroy(app->renderpass);
    // FREE(app->renderpass);

    // Destroy the renderer.
    dvz_renderer_destroy(app->rd);

    // Destroy the GPU (including the physical device).
    dvz_gpu_destroy(app->gpu);

    // Destroy the host (but NOT the VkInstance thanks to host->no_instance_destroy).
    dvz_host_destroy(app->host);

    // Destroy the VkInstance using Qt.
    app->inst->destroy();
    delete app->inst;

    FREE(app);
}

EXTERN_C_OFF



#else

// Fallbacks.
DvzQtApp* dvz_qt_app(QApplication* qapp, int flags) { return NULL; };
DvzQtWindow* dvz_qt_window(DvzQtApp* app) { return NULL; }
void dvz_qt_submit(DvzQtApp* app, DvzBatch* batch) { return; }
DvzBatch* dvz_qt_batch(DvzQtApp* app) { return NULL; }
void dvz_qt_app_destroy(DvzQtApp* app) { return; }

#endif
