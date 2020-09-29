#ifndef VKY_GLFW_HEADER
#define VKY_GLFW_HEADER

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <GLFW/glfw3.h>

#include "../include/visky/visky.h"



VkyMouseButton vky_get_glfw_mouse_button(GLFWwindow* window);

void vky_get_glfw_mouse_pos(GLFWwindow* window, vec2 pos);

void vky_get_glfw_keyboard(GLFWwindow* window, VkyKey*, uint32_t*);

void vky_glfw_wait(VkyCanvas* canvas);

VkyCanvas* vky_create_glfw_canvas(VkyApp* app, uint32_t, uint32_t);

void vky_glfw_begin_frame(VkyCanvas* canvas);

void vky_glfw_end_frame(VkyCanvas* canvas);

void vky_run_glfw_app(VkyApp* app);

void vky_glfw_stop_app(VkyApp* app);

#endif
