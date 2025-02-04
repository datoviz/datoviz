/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Qt app                                                                                       */
/*************************************************************************************************/

#if HAS_QT

#include <QFile>
#include <QGuiApplication>
#include <QVersionNumber>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <QVulkanWindow>

#include "datoviz.h"
#include "datoviz_protocol.h"
#include "host.h"
#include "qtapp.hpp"
#include "render_utils.h"
#include "renderer.h"
#include "vklite.h"



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
    VulkanRenderer(QVulkanWindow* window, DvzQtApp* app) : m_window(window), m_app(app) {}

    void initResources() override
    {
        QVulkanInstance* inst = m_window->vulkanInstance();
        m_devFuncs = inst->deviceFunctions(m_window->device());
    }

    void initSwapChainResources() override {}

    void releaseSwapChainResources() override {}

    void releaseResources() override {}

    void startNextFrame() override
    {
        VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();


        // Begin render pass with a clear blue color
        VkClearValue clearColor = {};
        clearColor.color.float32[0] = 0.0f; // Red
        clearColor.color.float32[1] = 0.0f; // Green
        clearColor.color.float32[2] = 1.0f; // Blue
        clearColor.color.float32[3] = 1.0f; // Alpha

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        // renderPassInfo.renderPass = m_app->renderpass->renderpass;
        renderPassInfo.renderPass = m_window->defaultRenderPass();
        renderPassInfo.framebuffer = m_window->currentFramebuffer();
        renderPassInfo.renderArea.offset = {0, 0};

        QSize swapChainSize = m_window->swapChainImageSize();
        renderPassInfo.renderArea.extent = {
            static_cast<uint32_t>(swapChainSize.width()),
            static_cast<uint32_t>(swapChainSize.height())};

        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        m_devFuncs->vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        m_devFuncs->vkCmdEndRenderPass(cmdBuf);


        m_window->frameReady();
        m_window->requestUpdate();
    }

private:
    QVulkanWindow* m_window;
    QVulkanDeviceFunctions* m_devFuncs;
    DvzQtApp* m_app;
};



class VulkanWindow : public QVulkanWindow
{
public:
    VulkanWindow(DvzQtApp* app) : m_app(app) {}

    QVulkanWindowRenderer* createRenderer() override { return new VulkanRenderer(this, m_app); }

private:
    DvzQtApp* m_app;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



DvzQtApp* dvz_qt_app(QApplication* qapp, int flags)
{
    // ANN(qapp);

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
    app->gpu = make_gpu(app->host);
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
    app->rd = dvz_renderer(app->gpu, flags | DVZ_RENDERER_FLAGS_NO_WORKSPACE);
    ANN(app->rd);

    // Manually create a renderpass.
    cvec4 clear_color = {0};
    default_clear_color(flags, clear_color);
    app->renderpass = (DvzRenderpass*)calloc(1, sizeof(DvzRenderpass));
    *app->renderpass = dvz_gpu_renderpass(app->gpu, clear_color, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Create a batch.
    app->batch = dvz_batch();
    ANN(app->batch);

    return app;
}



QVulkanWindow* dvz_qt_window(DvzQtApp* app)
{
    ANN(app);
    VulkanWindow* window = new VulkanWindow(app);
    window->setVulkanInstance(app->inst);
    return (QVulkanWindow*)window;
}



void dvz_qt_app_destroy(DvzQtApp* app)
{
    // Destroy the batch.
    dvz_batch_destroy(app->batch);

    // Destroy and free the renderpass.
    dvz_renderpass_destroy(app->renderpass);
    FREE(app->renderpass);

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



#else
// Fallbacks.
DvzQtApp* dvz_qt_app(QApplication* qapp) { return NULL };
QVulkanWindow* dvz_qt_window(DvzQtApp* app) { return NULL; }
void dvz_qt_app_destroy(DvzQtApp* app) { return; }
#endif

EXTERN_C_OFF
