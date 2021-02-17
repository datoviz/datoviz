/*************************************************************************************************/
/*  Example of a standalone C++ application using the library and Dear ImGUI directly.           */
/*************************************************************************************************/

#include <IconsFontAwesome.h> // used for FontAwesome icons
#include <datoviz/datoviz.h>  // import Datoviz
#include <imgui/imgui.h>      // import Dear ImGui

static char buf[256];
static float f;

// This is where the Dear ImGui code goes:
void _gui_callback(DvzCanvas* canvas, DvzEvent ev)
{
    // Code below coming directly from: https://github.com/ocornut/imgui

    ImGui::Text("Hello, world %d", 123);
    if (ImGui::Button("Save"))
        printf("Saving!\n");
    ImGui::InputText("string", buf, IM_ARRAYSIZE(buf));
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
}

int main(int argc, char** argv)
{
    // Create an app as usual.
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);

    // NOTE: we must set the IMGUI flag in order to use Dear ImGUI.
    DvzCanvas* canvas = dvz_canvas(gpu, 1280, 1024, DVZ_CANVAS_FLAGS_IMGUI);

    // Event callback used for Dear ImGUI. It is called at every frame. The callback can call Dear
    // ImGui functions to create dialogs.
    dvz_event_callback(canvas, DVZ_EVENT_IMGUI, 0, DVZ_EVENT_MODE_SYNC, _gui_callback, NULL);

    // Destroy the app.
    dvz_app_run(app, 0);
    dvz_app_destroy(app);
    return 0;
}
