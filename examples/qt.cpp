#include <visky/visky.h>

#if HAS_QT

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <QVulkanWindow>



static VkCommandBuffer vky_begin_qt_render_pass(
    QVulkanWindow* m_window, QVulkanDeviceFunctions* m_devFuncs, VkClearColorValue color)
{
    VkClearDepthStencilValue clearDS = {1.0f, 0};
    VkClearValue clearValues[2];
    memset(clearValues, 0, sizeof(clearValues));
    clearValues[0].color = color;
    clearValues[1].depthStencil = clearDS;

    VkRenderPassBeginInfo rpBeginInfo;
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = m_window->defaultRenderPass();
    rpBeginInfo.framebuffer = m_window->currentFramebuffer();
    const QSize sz = m_window->swapChainImageSize();
    rpBeginInfo.renderArea.extent.width = (uint32_t)sz.width();
    rpBeginInfo.renderArea.extent.height = (uint32_t)sz.height();
    rpBeginInfo.clearValueCount = 2;
    rpBeginInfo.pClearValues = clearValues;
    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    m_devFuncs->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    return cmdBuf;
}



class VulkanRenderer : public QVulkanWindowRenderer
{
public:
    VulkanRenderer(QVulkanWindow* w);
    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void startNextFrame() override;

private:
    QVulkanWindow* m_window;
    QVulkanDeviceFunctions* m_devFuncs;
    VkyApp* m_app; // TODO: in app.c, allocate app and gpu
    VkyGpu* m_gpu;
    VkyCanvas canvas;
};


class VulkanWindow : public QVulkanWindow
{
public:
    QVulkanWindowRenderer* createRenderer() override;
    void setVisky(VkyApp* app, VkyGpu* gpu, VkyCanvas* canvas);

private:
    QVulkanWindowRenderer* renderer;
};

QVulkanWindowRenderer* VulkanWindow::createRenderer()
{
    renderer = new VulkanRenderer(this);
    return renderer;
}

VulkanRenderer::VulkanRenderer(QVulkanWindow* w) : m_window(w) {}

void VulkanRenderer::initResources()
{
    m_devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    QVulkanFunctions* f = m_window->vulkanInstance()->functions();

    VkyGpu gpu = {0}; // TODO: *m_gpu;

    // Construct the GPU struct.
    gpu.instance = m_window->vulkanInstance()->vkInstance();
    gpu.physical_device = m_window->physicalDevice();
    gpu.device = m_window->device();
    gpu.device_properties = *m_window->physicalDeviceProperties();
    f->vkGetPhysicalDeviceFeatures(gpu.physical_device, &gpu.device_features);
    f->vkGetPhysicalDeviceMemoryProperties(gpu.physical_device, &gpu.memory_properties);

    // Queue families.
    VkyQueueFamilyIndices indices = {0, 0, 0};
    bool graphics_found = false, present_found = false;
    uint32_t queue_family_count = 0;
    f->vkGetPhysicalDeviceQueueFamilyProperties(gpu.physical_device, &queue_family_count, NULL);
    ASSERT(queue_family_count > 0);
    VkQueueFamilyProperties queueFamilies[50];
    f->vkGetPhysicalDeviceQueueFamilyProperties(
        gpu.physical_device, &queue_family_count, queueFamilies);

    for (uint32_t i = 0; i < queue_family_count; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphics_family = i;
            graphics_found = true;
        }
        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            indices.compute_family = i;
        }

        VkBool32 presentSupport =
            m_window->vulkanInstance()->supportsPresent(gpu.physical_device, i, m_window);
        if (presentSupport)
        {
            indices.present_family = i;
            present_found = true;
        }

        if (graphics_found && present_found)
        {
            break;
        }
    }

    gpu.queue_indices = indices;
    gpu.graphics_queue = m_window->graphicsQueue();
    m_devFuncs->vkGetDeviceQueue(gpu.device, indices.present_family, 0, &gpu.present_queue);

    gpu.command_pool = m_window->graphicsCommandPool();
    gpu.image_count = (uint32_t)m_window->swapChainImageCount();
    gpu.image_format = m_window->colorFormat();

    // Update the struct hold in the class.
    // *m_gpu = gpu;  // TODO

    // Construct the Canvas struct.
    canvas.window = m_window;
    canvas.gpu = m_gpu;
    // TODO: create scene and event_controller
    canvas.dpi_factor = 1.0; // TODO
    canvas.surface = m_window->vulkanInstance()->surfaceForWindow(m_window);

    // Canvas extent.
    QSize size = m_window->swapChainImageSize();
    // canvas.extent = {};
    // canvas.window_size.w = canvas.extent.width;
    // canvas.window_size.h = canvas.extent.height;
    canvas.size.framebuffer_width = canvas.size.window_width = (uint32_t)size.width();
    canvas.size.framebuffer_height = canvas.size.window_height = (uint32_t)size.height();
    canvas.image_count = gpu.image_count;
    canvas.render_pass = m_window->defaultRenderPass();

    canvas.image_format = gpu.image_format;
    canvas.depth_format = m_window->depthStencilFormat();
    canvas.depth_image = m_window->depthStencilImage();
    canvas.depth_image_view = m_window->depthStencilImageView();
    // NOTE: no depth_image_memory and swapchain
}

void VulkanRenderer::initSwapChainResources()
{
    QSize size = m_window->swapChainImageSize();
    // canvas.extent = {(uint32_t)size.width(), (uint32_t)size.height()};
    // canvas.window_size.w = canvas.extent.width;
    // canvas.window_size.h = canvas.extent.height;
    canvas.size.framebuffer_width = canvas.size.window_width = (uint32_t)size.width();
    canvas.size.framebuffer_height = canvas.size.window_height = (uint32_t)size.height();
}

void VulkanRenderer::releaseSwapChainResources() {}

void VulkanRenderer::releaseResources() {}

void VulkanRenderer::startNextFrame()
{
    VkCommandBuffer cmdBuf = vky_begin_qt_render_pass(m_window, m_devFuncs, {{1, 0, 0, 1}});

    // Do nothing else. We will just clear to green, changing the component on
    // every invocation. This also helps verifying the rate to which the thread
    // is throttled to. (The elapsed time between startNextFrame calls should
    // typically be around 16 ms. Note that rendering is 2 frames ahead of what
    // is displayed.)


    // // Bind the vertex buffer.
    // vky_bind_vertex_buffer(cmd_buf, vertex_buffer, 0);

    // // Bind the graphics pipeline.
    // vky_bind_graphics_pipeline(cmd_buf, &pipeline);

    // // Set the full viewport.
    // vky_set_viewport(cmd_buf, 0, 0, canvas->size.framebuffer_width,
    // canvas->size.framebuffer_height);

    // // Draw 3 vertices = 1 triangle.
    // vky_draw(cmd_buf, 0, 3);



    m_devFuncs->vkCmdEndRenderPass(cmdBuf);

    m_window->frameReady();
    m_window->requestUpdate(); // render continuously, throttled by the presentation rate
}

#if GCC
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#elif CLANG
#pragma clang diagnostic ignored "-Wmissing-declarations"
#endif
Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")
#if GCC
#pragma GCC diagnostic pop
#elif CLANG
#pragma clang diagnostic pop
#endif


int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));
    QVulkanInstance inst;

    inst.setLayers(
        QByteArrayList() << "VK_LAYER_GOOGLE_threading"
                         << "VK_LAYER_LUNARG_parameter_validation"
                         << "VK_LAYER_LUNARG_object_tracker"
                         << "VK_LAYER_LUNARG_core_validation"
                         << "VK_LAYER_LUNARG_image"
                         << "VK_LAYER_LUNARG_swapchain"
                         << "VK_LAYER_GOOGLE_unique_objects");

    if (!inst.create())
        qFatal("Failed to create Vulkan instance: %d", inst.errorCode());

    VulkanWindow w;
    w.setVulkanInstance(&inst);

    w.resize(1024, 768);
    w.show();

    return app.exec();
}



#else

// No Qt support

int main(int argc, char* argv[]) { log_error("visky was not built with Qt5 support"); }



#endif
