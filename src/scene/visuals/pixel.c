/*************************************************************************************************/
/*  Pixel                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/pixel.h"
#include "fileio.h"
#include "request.h"
#include "scene/baker.h"
#include "scene/graphics.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzPixel* dvz_pixel(DvzRequester* rqr, int flags)
{
    ANN(rqr);

    DvzPixel* pixel = (DvzPixel*)calloc(1, sizeof(DvzPixel));
    pixel->rqr = rqr;
    pixel->flags = flags;

    pixel->baker = dvz_baker(rqr, 0);

    dvz_obj_init(&pixel->obj);
    return pixel;
}



void dvz_pixel_position(DvzPixel* pixel, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(pixel);
    if (!dvz_obj_is_created(&pixel->obj))
        dvz_pixel_create(pixel);
    dvz_baker_data(pixel->baker, 0, first, count, (void*)values);
}



void dvz_pixel_color(DvzPixel* pixel, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(pixel);
    if (!dvz_obj_is_created(&pixel->obj))
        dvz_pixel_create(pixel);
    dvz_baker_data(pixel->baker, 1, first, count, (void*)values);
}



void dvz_pixel_create(DvzPixel* pixel)
{
    ANN(pixel);
    log_debug("creating pixel visual");

    // NOTE: for now static assignments
    DvzRequester* rqr = pixel->rqr;
    ANN(rqr);

    // Create the graphics object.
    DvzRequest req = dvz_create_graphics(pixel->rqr, DVZ_GRAPHICS_CUSTOM, 0);
    DvzId graphics_id = req.id;
    pixel->graphics_id = graphics_id;

    // Load shaders.
    unsigned long size = 0;
    unsigned char* buffer = dvz_resource_shader("graphics_basic_vert", &size);
    dvz_set_spirv(rqr, graphics_id, DVZ_SHADER_VERTEX, size, buffer);
    buffer = dvz_resource_shader("graphics_basic_frag", &size);
    dvz_set_spirv(rqr, graphics_id, DVZ_SHADER_FRAGMENT, size, buffer);

    // Primitive topology.
    dvz_set_primitive(rqr, graphics_id, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST);

    // Polygon mode.
    dvz_set_polygon(rqr, graphics_id, DVZ_POLYGON_MODE_FILL);


    // Set up the baker.
    DvzBaker* baker = pixel->baker;
    dvz_baker_vertex(baker, 0, sizeof(DvzPixelVertex));
    dvz_baker_attr(baker, 0, 0, offsetof(DvzPixelVertex, pos), sizeof(vec3));
    dvz_baker_attr(baker, 1, 0, offsetof(DvzPixelVertex, color), sizeof(cvec4));
    dvz_baker_slot(baker, 0, sizeof(DvzMVP));
    dvz_baker_slot(baker, 1, sizeof(DvzViewport));
    dvz_baker_create(baker, 10000); // DEBUG


    // Vertex binding.
    dvz_set_vertex(rqr, graphics_id, 0, sizeof(DvzPixelVertex), DVZ_VERTEX_INPUT_RATE_VERTEX);

    // Vertex attrs.
    dvz_set_attr(
        rqr, graphics_id, 0, 0, DVZ_FORMAT_R32G32B32_SFLOAT, offsetof(DvzPixelVertex, pos));
    dvz_set_attr(
        rqr, graphics_id, 0, 1, DVZ_FORMAT_R8G8B8A8_UNORM, offsetof(DvzPixelVertex, color));

    // Descriptor slots.
    dvz_set_slot(rqr, graphics_id, 0, DVZ_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dvz_set_slot(rqr, graphics_id, 1, DVZ_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // Bindings.
    dvz_bind_vertex(rqr, graphics_id, 0, baker->vertex_bindings[0].dual.dat, 0);
    dvz_bind_dat(rqr, graphics_id, 0, baker->descriptors[0].dual.dat);
    dvz_bind_dat(rqr, graphics_id, 1, baker->descriptors[1].dual.dat);

    // MVP data.
    DvzMVP mvp = dvz_mvp_default();
    dvz_baker_uniform(baker, 0, sizeof(DvzMVP), &mvp);

    // Viewport data.
    // TODO: pass viewport info
    DvzViewport viewport = dvz_viewport_default(0, 0);
    dvz_baker_uniform(baker, 1, sizeof(DvzViewport), &viewport);

    dvz_obj_created(&pixel->obj);
}



void dvz_pixel_draw(DvzPixel* pixel, DvzId canvas, uint32_t first, uint32_t count, int flags)
{
    ANN(pixel);

    // Emit the dat update commands.
    dvz_pixel_update(pixel);

    // Emit the record commands.
    dvz_record_draw(pixel->rqr, canvas, pixel->graphics_id, first, count, 0, 1);
}



void dvz_pixel_update(DvzPixel* pixel)
{
    ANN(pixel);
    ANN(pixel->baker);
    dvz_baker_update(pixel->baker);
}



void dvz_pixel_destroy(DvzPixel* pixel)
{
    ANN(pixel);

    dvz_baker_destroy(pixel->baker);

    FREE(pixel);
}
