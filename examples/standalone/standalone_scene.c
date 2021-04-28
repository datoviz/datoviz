/*************************************************************************************************/
/*  Example of a standalone application using the library.                                       */
/*************************************************************************************************/

/// Import the library public header. The compiler needs to be passed the path to the headers and
/// shared library, see build.sh.
#include <datoviz/datoviz.h>



int main(int argc, char** argv)
{
    // Create a singleton application with a GLFW backend.
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);

    // Automatically choose the most capable GPU.
    DvzGpu* gpu = dvz_gpu_best(app);

    // Create a new canvas with the size specified. The last argument is for optional flags.
    DvzCanvas* canvas = dvz_canvas(gpu, 1280, 1024, 0);

    // We use a white background color (r, g, b floating-point values in [0, 1]).
    dvz_canvas_clear_color(canvas, 1, 1, 1);

    // Create a new scene, which allows to define several subplots (panels) organized within a
    // grid. Here we just use a single panel spanning the entire canvas.
    DvzScene* scene = dvz_scene(canvas, 1, 1);

    // Get the panel at row 0, column 0, and initialize it with an axes 2D controller.
    // The last argument is for optional flags.
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_AXES_2D, 0);

    // Create a new "marker" visual within the panel.
    // The last argument is for optional flags.
    DvzVisual* visual = dvz_scene_visual(panel, DVZ_VISUAL_MARKER, 0);

    // Visual data.
    {
        // Prepare the data for the marker visual:
        // - positions (three double-precision floating point numbers for x, y, z)
        // - color (four bytes for r, g, b, a)
        // - size (a single precision floating point number with the marker size, in pixels)
        // There are many other settable visual properties, refer to the visual reference guide.
        const uint32_t N = 100000;
        dvec3* pos = (dvec3*)calloc(N, sizeof(dvec3));
        cvec4* color = (cvec4*)calloc(N, sizeof(cvec4));
        float* size = (float*)calloc(N, sizeof(float));
        for (uint32_t i = 0; i < N; i++)
        {
            // Random gaussian position.
            pos[i][0] = dvz_rand_normal();
            pos[i][1] = dvz_rand_normal();
            // Color: random value with a colormap.
            dvz_colormap_scale(DVZ_CMAP_VIRIDIS, dvz_rand_float(), 0, 1, color[i]);
            // A bit of transparency via the alpha channel.
            color[i][3] = 196;
            // Random marker size.
            size[i] = 2 + 38 * dvz_rand_float();
        }

        // We link our data to the visual properties.
        dvz_visual_data(visual, DVZ_PROP_POS, 0, N, pos);
        dvz_visual_data(visual, DVZ_PROP_COLOR, 0, N, color);
        dvz_visual_data(visual, DVZ_PROP_MARKER_SIZE, 0, N, size);

        // We free the memory of the arrays we've created.
        FREE(pos);
        FREE(color);
        FREE(size);
    }



// NOTE: ignore this part, it is exclusively for the building and testing process.
#ifdef AUTOMATED
    log_info("saving standalone scene screenshot");

    // Instead of opening a live interactive canvas, save a screenshot and close the canvas
    // immediately.
    dvz_app_run(app, 5);
    dvz_screenshot_file(canvas, "../../docs/images/screenshots/standalone_scene.png");
#else
    log_info("running standalone scene example");

    // We run the application. The last argument is the number of frames to run, or 0 for infinite
    // loop (stop when escape is pressed or when the window is closed).
    dvz_app_run(app, 0);
#endif



    // We need to clean up all objects handled by Datoviz at the end.
    dvz_app_destroy(app);

    return 0;
}
