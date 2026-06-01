#pragma once

#include <stdint.h>

#ifdef _WIN32
#define DVZ_QTBRIDGE_API __declspec(dllexport)
#elif defined(__GNUC__)
#define DVZ_QTBRIDGE_API __attribute__((visibility("default")))
#else
#define DVZ_QTBRIDGE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define DVZ_QTBRIDGE_ABI_VERSION 1u

DVZ_QTBRIDGE_API uint32_t dvz_qtbridge_abi_version(void);
DVZ_QTBRIDGE_API const char* dvz_qtbridge_qt_version(void);

DVZ_QTBRIDGE_API int
dvz_qtbridge_qvulkan_set_vk_instance(uintptr_t qvulkan_instance, uintptr_t vk_instance);
DVZ_QTBRIDGE_API uintptr_t dvz_qtbridge_qvulkan_vk_instance(uintptr_t qvulkan_instance);

#ifdef __cplusplus
}
#endif
