#include "qt_bridge.h"

#include <QVulkanInstance>
#include <QtGlobal>


uint32_t dvz_qtbridge_abi_version(void)
{
    return DVZ_QTBRIDGE_ABI_VERSION;
}


const char* dvz_qtbridge_qt_version(void)
{
    return qVersion();
}


int dvz_qtbridge_qvulkan_set_vk_instance(uintptr_t qvulkan_instance, uintptr_t vk_instance)
{
    if (qvulkan_instance == 0)
        return -1;
    if (vk_instance == 0)
        return -2;

    auto* qt_instance = reinterpret_cast<QVulkanInstance*>(qvulkan_instance);
    qt_instance->setVkInstance(reinterpret_cast<VkInstance>(vk_instance));
    return 0;
}


uintptr_t dvz_qtbridge_qvulkan_vk_instance(uintptr_t qvulkan_instance)
{
    if (qvulkan_instance == 0)
        return 0;

    auto* qt_instance = reinterpret_cast<QVulkanInstance*>(qvulkan_instance);
    return reinterpret_cast<uintptr_t>(qt_instance->vkInstance());
}
