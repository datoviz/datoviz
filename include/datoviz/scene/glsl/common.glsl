/*************************************************************************************************/
/*  Constants and macros                                                                         */
/*************************************************************************************************/

// Transform flags.
#define DVZ_SPECIALIZATION_TRANSFORM 16
#define DVZ_TRANSFORM_FIXED_X        0x1
#define DVZ_TRANSFORM_FIXED_Y        0x2
#define DVZ_TRANSFORM_FIXED_Z        0x4

// Viewport flags.
#define DVZ_SPECIALIZATION_VIEWPORT 17
#define DVZ_VIEWPORT_CLIP_INNER     0x1
#define DVZ_VIEWPORT_CLIP_OUTER     0x2
#define DVZ_VIEWPORT_CLIP_BOTTOM    0x4
#define DVZ_VIEWPORT_CLIP_LEFT      0x8

// Specialization constants.
layout(constant_id = DVZ_SPECIALIZATION_TRANSFORM) const int TRANSFORM_FLAGS = 0;
layout(constant_id = DVZ_SPECIALIZATION_VIEWPORT) const int VIEWPORT_FLAGS = 0;


// colormaps
#define CPAL032_OFS     240
#define CPAL032_SIZ     32
#define CPAL032_PER_ROW 8

// number of common bindings
#define USER_BINDING 2

// NOTE: needs to be a macro and not a function so that it can be safely included in both
// vertex and fragment shaders (discard is forbidden in the vertex shader)
#define CLIP                                                                                      \
    if ((VIEWPORT_FLAGS & DVZ_VIEWPORT_CLIP_INNER) > 0)                                           \
        if (!clip_viewport(gl_FragCoord.xy))                                                      \
            discard;                                                                              \
    if ((VIEWPORT_FLAGS & DVZ_VIEWPORT_CLIP_OUTER) > 0)                                           \
        if (clip_viewport(gl_FragCoord.xy))                                                       \
            discard;                                                                              \
    if ((VIEWPORT_FLAGS & DVZ_VIEWPORT_CLIP_BOTTOM) > 0)                                          \
        if (clip_viewport(gl_FragCoord.xy, 0))                                                    \
            discard;                                                                              \
    if ((VIEWPORT_FLAGS & DVZ_VIEWPORT_CLIP_LEFT) > 0)                                            \
        if (clip_viewport(gl_FragCoord.xy, 1))                                                    \
            discard;


#define sum(x) (dot(x, vec4(1, 1, 1, 1)))



/*************************************************************************************************/
/*  Common bindings                                                                              */
/*************************************************************************************************/

layout(std140, binding = 0) uniform MVP
{
    mat4 model;
    mat4 view;
    mat4 proj;
    // float time;
}
mvp;

struct VkViewport
{
    float x, y, w, h, dmin, dmax;
};

layout(std140, binding = 1) uniform Viewport
{
    VkViewport viewport; // Vulkan viewport
    vec4 margins;        // margins

    uvec2 offset_screen; // screen coordinates
    uvec2 size_screen;   // screen coordinates

    uvec2 offset; // framebuffer coordinates
    uvec2 size;   // framebuffer coordinates

    // Options
    int flags;
}
viewport;



/*************************************************************************************************/
/*  Misc functions                                                                               */
/*************************************************************************************************/

vec4 to_vulkan(vec4 tr)
{
    // HACK: we transform from OpenGL conventional coordinate system to Vulkan
    // This allows us to use MVP matrices in OpenGL conventions.
    tr.y = -tr.y;              // Vulkan swaps top and bottom in its device coordinate system.
    tr.z = .5 * (tr.z + tr.w); // depth is [-1, 1] in OpenGL but [0, 1] in Vulkan
    return tr;
}



mat4 get_ortho_matrix(vec2 size)
{
    // The orthographic projection is:
    // 2/w            -1
    //       2/h      -1
    //            1    0
    //                 1
    mat4 ortho = mat4(1.0);

    // WARNING: column-major order (=FORTRAN order, columns first)
    ortho[0][0] = 2. / size.x;
    ortho[1][1] = 2. / size.y;
    ortho[2][2] = 1.;

    // ortho[3] = vec4(-1, -1, 0, 1);
    ortho[3] = vec4(0, 0, 0, 1);

    return ortho;
}



mat4 get_ortho_matrix()
{
    float w = viewport.size.x;
    float h = viewport.size.y;
    float mt = viewport.margins.x;
    float mr = viewport.margins.y;
    float mb = viewport.margins.z;
    float ml = viewport.margins.w;

    vec2 size = vec2(w - ml - mr, h - mt - mb);

    return get_ortho_matrix(size);
}



mat4 get_translation_matrix(vec2 xy)
{
    // The translation matrix is:
    //  1        x
    //     1     y
    //        1  0
    //           1
    // NOTE: column-major order
    return mat4(
        vec4(1, 0, 0, 0), //
        vec4(0, 1, 0, 0), //
        vec4(0, 0, 1, 0), //
        vec4(xy, 0, 1));
}



mat4 get_rotation_matrix(vec3 axis, float angle)
{
    if (length(axis) == 0)
        return mat4(1);

    axis = normalize(axis);
    float c = cos(angle);
    float s = sin(angle);
    float t = 1.0 - c;
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    return mat4(
        vec4(t * x * x + c, t * x * y + s * z, t * x * z - s * y, 0.0), //
        vec4(t * x * y - s * z, t * y * y + c, t * y * z + s * x, 0.0), //
        vec4(t * x * z + s * y, t * y * z - s * x, t * z * z + c, 0.0), //
        vec4(0.0, 0.0, 0.0, 1.0));
}



/*************************************************************************************************/
/*  Transforms                                                                                   */
/*************************************************************************************************/

vec4 transform_mvp(vec3 pos)
{
    mat4 MVP = mvp.proj * mvp.view * mvp.model;
    vec4 tr = MVP * vec4(pos, 1.0);
    return tr;
}



vec4 transform_fixed(vec4 tr, vec3 pos)
{
    // Fixed axes.
    if ((TRANSFORM_FLAGS & DVZ_TRANSFORM_FIXED_X) > 0)
        tr.x = pos.x;
    if ((TRANSFORM_FLAGS & DVZ_TRANSFORM_FIXED_Y) > 0)
        tr.y = pos.y;
    if ((TRANSFORM_FLAGS & DVZ_TRANSFORM_FIXED_Z) > 0)
        tr.z = pos.z;
    return tr;
}


vec4 transform_margins(vec4 tr)
{
    // Margins
    float w = viewport.size.x;
    float h = viewport.size.y;
    float mt = viewport.margins.x;
    float mr = viewport.margins.y;
    float mb = viewport.margins.z;
    float ml = viewport.margins.w;
    float a = 1;
    float b = 0;

    // horizontal margins
    if (w > 0)
    {
        a = 1 - (ml + mr) / w;
        b = (ml - mr) / w;
        tr.x = a * tr.x + b;
    }

    // vertical margins
    if (h > 0)
    {
        a = 1 - (mb + mt) / h;
        b = (mb - mt) / h;
        tr.y = a * tr.y + b;
    }

    return tr;
}



vec4 transform_shift(vec4 tr, vec2 shift)
{
    float w = viewport.size.x;
    float h = viewport.size.y;

    // pixel shift.
    if (w > 0 && h > 0)
        tr.xy += (2 * shift / viewport.size);
    return tr;
}



vec4 transform(vec3 pos, vec2 shift)
{
    vec4 tr = transform_mvp(pos);
    tr = transform_fixed(tr, pos);
    tr = transform_margins(tr);
    tr = transform_shift(tr, shift);
    tr = to_vulkan(tr);
    return tr;
}



// vec4 transform(vec3 pos, vec2 shift) { return transform(pos, shift, 0); }



// vec4 transform(vec3 pos, uint transform_flags)
// {
//     return transform(pos, vec2(0, 0), transform_flags);
// }



vec4 transform(vec3 pos) { return transform(pos, vec2(0, 0)); }



/*************************************************************************************************/
/*  Clipping                                                                                     */
/*************************************************************************************************/

bool clip_viewport(vec2 frag_coords)
{
    vec2 uv = frag_coords - viewport.offset;
    return (
        uv.y < 0 + viewport.margins.x || uv.x > viewport.size.x - viewport.margins.y ||
        uv.y > viewport.size.y - viewport.margins.z || uv.x < 0 + viewport.margins.w);
}



bool clip_viewport(vec2 frag_coords, int coord)
{
    vec2 uv = frag_coords - viewport.offset;

    float w = viewport.size.x;
    float h = viewport.size.y;

    float top = viewport.margins.x;
    float right = viewport.margins.y;
    float bottom = viewport.margins.z;
    float left = viewport.margins.w;

    // Discard top and right edges.
    // NOTE: extra margin: 10, 30px
    // if (uv.y < top - 10.0 || uv.x > w - right + 30.0)
    //     discard;

    // Bottom-left corner: discard along the diagonal
    if (uv.x < left && uv.y > h - bottom)
    {
        vec2 q0 = vec2(left, bottom);
        vec2 q1 = vec2(uv.x, h - uv.y);
        float s = (q0.x * q1.y - q0.y * q1.x);
        float eps = left * 5;
        if ((s > -eps && coord < .5) || (s < eps && coord > .5))
            return true;
    }
    return false;
}



/*************************************************************************************************/
/*  Colormap                                                                                     */
/*************************************************************************************************/

vec4 unpack_color(vec2 uv)
{
    // Return a vec4 R8G8B8A8 color from a vec2 texcoords where the first component is <0, and the
    // second one is a float packing 3 uint8 (RGB).
    // TODO: use the first float (-1 for now) for the alpha value?
    vec4 out_color = vec4(1);
    out_color.x = mod(uv.x, 256.0);
    out_color.y = mod(floor(uv.x / 256.0), 256.0);
    out_color.z = mod(floor(uv.x / 65536.0), 256.0);
    out_color /= 256.0;
    return out_color;
}



ivec2 colormap_idx(int cmap, int value)
{
    int row = 0, col = 0;
    if (cmap >= CPAL032_OFS)
    {
        // For 32-color palettes, we need to alter the cmap and value.
        row = (CPAL032_OFS + (cmap - CPAL032_OFS) / CPAL032_PER_ROW);
        col = CPAL032_SIZ * ((cmap - CPAL032_OFS) % CPAL032_PER_ROW) + value;
    }
    else
    {
        row = cmap;
        col = value;
    }
    return ivec2(row, col);
}



vec2 colormap_uv(int cmap, int value)
{
    ivec2 ij = colormap_idx(cmap, value);
    return vec2((ij[1] + .5) / 256., (ij[0] + .5) / 256.);
}



vec4 colormap_fetch(sampler2D tex, int cmap, float value)
{
    int ivalue = int(256 * value);
    vec2 uv = colormap_uv(cmap, ivalue);
    // return vec4(uv, 0, 1);
    return texture(tex, uv);
}



float transfer(float x, vec4 xcoefs, vec4 ycoefs)
{
    if (x < xcoefs.x)
        return ycoefs.x;
    else if (x < xcoefs.y)
        return ycoefs.x + (ycoefs.y - ycoefs.x) / (xcoefs.y - xcoefs.x) * (x - xcoefs.x);
    else if (x < xcoefs.z)
        return ycoefs.y + (ycoefs.z - ycoefs.y) / (xcoefs.z - xcoefs.y) * (x - xcoefs.y);
    else if (x < xcoefs.w)
        return ycoefs.z + (ycoefs.w - ycoefs.z) / (xcoefs.w - xcoefs.z) * (x - xcoefs.z);
    else
        return ycoefs.w;
}
