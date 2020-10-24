#include "../include/visky/visky.h"
BEGIN_INCL_NO_WARN
#include <imgui.h>
END_INCL_NO_WARN


static bool my_tool_active = true;
static vec3 my_color_prev;
static vec3 my_color;
static VkyVisual* visual;


static void fill_live_command_buffer(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    vky_imgui_newframe();

    // Disable Visky controller interaction when interacting with the GUI.
    vky_imgui_capture(canvas);

    // The following code is taken directly from https://github.com/ocornut/imgui#usage
    // for demonstration purposes.

    // Create a window called "My First Tool", with a menu bar.
    ImGui::Begin("My First Tool", &my_tool_active, ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open..", "Ctrl+O"))
            { /* Do stuff */
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            { /* Do stuff */
            }
            if (ImGui::MenuItem("Close", "Ctrl+W"))
            {
                my_tool_active = false;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Edit a color (stored as 3 floats)
    ImGui::ColorEdit3("Color", my_color);
    // Update the visual with the chosen color if the color has changed.
    if (memcmp(my_color, my_color_prev, sizeof(vec3)) != 0)
    {
        VkyColor color = vky_vec3_to_color(my_color);
        vky_visual_data_partial(visual, VKY_VISUAL_PROP_COLOR, 0, 0, 1, 1, &color);
        glm_vec3_copy(my_color, my_color_prev);
    }

    // Plot some values
    const float my_values[] = {0.2f, 0.1f, 1.0f, 0.5f, 0.9f, 2.2f};
    ImGui::PlotLines("Frame Times", my_values, IM_ARRAYSIZE(my_values));

    // Display contents in a scrolling region
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Important Stuff");
    ImGui::BeginChild("Scrolling");
    for (int n = 0; n < 50; n++)
        ImGui::Text("%04d: Some text", n);
    ImGui::EndChild();
    ImGui::End();

    vky_imgui_render(canvas, cmd_buf);
}


int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    vky_imgui_init(canvas);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);

    visual = vky_visual(scene, VKY_VISUAL_MESH_RAW, NULL, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    vec3 positions[] = {{-1, -1, 0}, {+1, -1, 0}, {0, +1, 0}};
    cvec4 colors[] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};

    vky_visual_data_set_size(visual, 3, 0, NULL, NULL);
    vky_visual_data(visual, VKY_VISUAL_PROP_POS_GPU, 0, 3, positions);
    vky_visual_data(visual, VKY_VISUAL_PROP_COLOR, 0, 3, colors);

    canvas->cb_fill_live_command_buffer = fill_live_command_buffer;

    vky_run_app(app);

    vky_imgui_destroy();
    vky_destroy_app(app);
    return 0;
}
