#include <visky/visky.h>

static const uint32_t path_count = 385;
static const uint32_t vertex_count_per_path = 30000;
static const uint32_t buf_vertex_count = 500;
static uint32_t vertex_offset = 0;
static int16_t* vertices = NULL;
static VkyBufferRegion* vertex_buffer;
static const char* filename = "data/misc/ephys.bin"; // raw binary file of int16 values
static int64_t data_offset = 0;
static FILE* f;
static bool play = true;

static void keyboard_callback(VkyCanvas* canvas, void* data)
{
    VkyKeyboard* keyboard = canvas->event_controller->keyboard;
    // Toggle play/pause by pressing a key.
    if (vky_is_key_modifier(keyboard->key))
        return;
    play = !play;
}

static void upload_data(VkyCanvas* canvas)
{
    if (!play)
        return;
    // Read the data from a file.
    uint32_t n = path_count * buf_vertex_count;
    VkDeviceSize vbuf_size = n * sizeof(int16_t);
    if (!f)
    {
        if (canvas->frame_count == 0)
            log_error("Could not find %s, falling back to random data.", filename);
        // Random data.
        for (uint32_t i = 0; i < n; i++)
        {
            vertices[i] = ((int16_t)rand_byte() - 128) / 4;
        }
    }
    else
    {
        fseek(f, data_offset, SEEK_SET);
        fread(vertices, sizeof(int16_t), n, f);
        ASSERT(vbuf_size == buf_vertex_count * path_count * sizeof(int16_t));
        ASSERT(ftell(f) == data_offset + (int64_t)vbuf_size);
        data_offset += (int64_t)vbuf_size;
    }

    // Upload the small CPU buffer to the large GPU buffer.
    vky_upload_buffer(
        *vertex_buffer, vertex_offset * path_count * sizeof(int16_t), vbuf_size, vertices);
    vertex_offset += buf_vertex_count;
    vertex_offset = vertex_offset % vertex_count_per_path;
}

int main()
{
    log_set_level_env();

    // Constrain the zooming out.
    vky_set_constant(VKY_PANZOOM_MIN_ZOOM_ID, 1);

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);

    // Set the panel controller.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);

    // Prepare the parameters.
    float denom = (float)(path_count - 1);
    VkyMultiRawPathParams params = {0};
    // Compute the y offsets.
    params.info[0] = (float)path_count;
    params.info[1] = (float)vertex_count_per_path;
    params.info[2] = .025 / denom; // scaling
    for (uint32_t i = 0; i < path_count; i++)
    {
        params.y_offsets[i / 4][i % 4] = -.9 + 1.8 * (float)i / denom;
        glm_vec4_copy((vec4){1, (float)i / denom, 0, 1}, params.colors[i]);
    }

    // Create the visual.
    VkyVisual* visual = vky_visual(scene, VKY_VISUAL_PATH_RAW_MULTI, &params, NULL);

    // Create the buffers.
    vky_add_vertex_buffer(canvas->gpu, vertex_count_per_path * path_count * sizeof(int16_t));
    vky_add_index_buffer(canvas->gpu, vertex_count_per_path * path_count * sizeof(uint32_t));
    vertices = calloc(buf_vertex_count * path_count, sizeof(int16_t));
    vertex_buffer = &visual->vertex_buffer;

    // Initialize the data.
    // NOTE: need to allocate on the heap as we may reach the maximum allocation size on the stack.
    int16_t* empty = calloc(vertex_count_per_path * path_count, sizeof(int16_t));
    visual->data.item_count = vertex_count_per_path * path_count;
    visual->data.items = empty;
    vky_visual_data_raw(visual);
    FREE(empty);

    // Add the visual to the panel.
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // The upload_data() function is called at every frame and update the data.
    f = fopen(filename, "rb");
    vky_add_frame_callback(canvas, upload_data, NULL);
    vky_add_frame_callback(canvas, keyboard_callback, NULL);

    vky_run_app(app);

    if (f)
        fclose(f);

    vky_destroy_app(app);
    FREE(vertices);
    return 0;
}
