#version 450
#include "common.glsl"
#include "colormaps.glsl"

#define STEP_SIZE 0.005
#define MAX_ITER 10 / STEP_SIZE

layout(std140, binding = USER_BINDING) uniform Params
{
    vec4 box_size;          /* size of the box containing the volume, in NDC */
    vec4 uvw0;              /* texture coordinates of the 2 corner points */
    vec4 uvw1;              /* texture coordinates of the 2 corner points */
    vec4 clip;              /* plane normal vector for volume slicing */
    vec2 transfer_xrange;   /* x coords of the endpoints of the transfer function */
    float color_coef;       /* scaling coefficient when fetching voxel color */
    // int cmap;               /* colormap */
}
params;

layout(binding = (USER_BINDING + 1)) uniform sampler3D tex_density;  // 3D vol with vox R density
// layout(binding = (USER_BINDING + 2)) uniform isampler3D tex_id;      // 3D vol with voxel id
layout(binding = (USER_BINDING + 2)) uniform sampler3D tex_colors;   // 3D vol with vox RGBA color
layout(binding = (USER_BINDING + 3)) uniform sampler1D tex_transfer; // transfer function

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_ray;

layout(location = 0) out vec4 out_color;
layout(location = 1) out ivec4 out_pick;



bool intersect_box(vec3 origin, vec3 dir, vec3 box_min, vec3 box_max, out float t0, out float t1)
{
    vec3 inv_r = 1.0 / dir;
    vec3 tbot = inv_r * (box_min-origin);
    vec3 ttop = inv_r * (box_max-origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);
    return t0 <= t1;
}



vec4 fetch_color(vec3 uvw) {
    float v = texture(tex_density, uvw).r;
    v = clamp(v, 0, 1);

    // Transfer function.
    float x0 = params.transfer_xrange.x;
    float x1 = params.transfer_xrange.y;
    if (x0 < x1)
        v = texture(tex_transfer, (v - x0) / (x1 - x0)).r;

    // Color component.
    vec4 color = params.color_coef * texture(tex_colors, uvw);

    // Alpha value: value.
    color.a = v;
    return color;
}



void main()
{
    CLIP

    mat4 mi = inverse(mvp.model);
    vec4 u_ = mi * vec4(normalize(in_ray), 1);
    vec3 u = u_.xyz / u_.w;
    vec4 o_ = mi * vec4(-mvp.view[3].xyz, 1);
    vec3 o = o_.xyz / o_.w;

    {
        // // Inner cube example.
        // float r = .25;
        // vec3 b0 = vec3(-r);
        // vec3 b1 = vec3(+r);
        // bool b = intersect_box(o, u, b0, b1);
        // float a = b ? .75 : .25;
        // out_color = vec4(0);
        // out_color.xyz *= a;
        // // Inner sphere example.
        // // float delta = pow(dot(u, o-c), 2) - (dot(o-c, o-c)-r*r);
    }

    float t0, t1;
    vec3 b0 = -params.box_size.xyz / 2;
    vec3 b1 = +params.box_size.xyz / 2;
    vec3 d = vec3(1) / (b1 - b0);
    intersect_box(o, u, b0, b1, t0, t1);
    if (t0 < 0 || t1 < 0) discard;

    {
        // Detect clipping plane.
        // vec3 c = params.clip.xyz;
        // vec3 dc = d * c;
        // float tclip = -params.clip.w - dot(dc, o - b0) / dot(dc, u);
        // vec3 uvw_clip = d * (o + u * tclip - b1);
        // if (t0 <= tclip && tclip <= t1) {
        //     // out_color = fetch_color(uvw_clip);
        //     out_color = vec4(1,0,0,1);
        //     return;
        // }
        // out_color = vec4((tclip - t0)/(t1-t0),1,0,1);
        // return;
    }

    vec3 ray_start = o + u * t0;
    vec3 ray_stop = o + u * t1;

    vec3 pos = ray_stop;
    vec3 dl = normalize(ray_start - ray_stop) * STEP_SIZE;
    float travel = distance(ray_start, ray_stop);
    float max_intensity = 0.0;
    vec3 uvw = vec3(0);
    vec4 s = vec4(0);
    vec4 acc = vec4(0);
    float alpha = 0;
    vec3 uvw_pick = vec3(0);
    bool in_clip = false;
    bool clip_front = false;

    for (int i = 0; i < MAX_ITER && travel > 0.0; ++i, pos += dl, travel -= STEP_SIZE) {
        // Normalize 3D pos within cube in [0,1]^3
        uvw = (pos - b0) * d;

        // Determine the position of the fragment compared to the clipping plane.
        if (dot(vec4(uvw, 1), params.clip) < 0) {
            in_clip = true;
            continue;
        }
        else if (i == 0) {
            clip_front = true;
        }
        uvw_pick = uvw;

        // Now, normalize between uvw0 and uvw1.
        uvw = params.uvw0.xyz + uvw * (params.uvw1 - params.uvw0).xyz;

        // Fetch the color from the 3D texture.
        s = fetch_color(uvw);
        alpha = s.a;
        acc = s + (1 - alpha) * acc;

        // MIP
        if (s.a > max_intensity) {
            max_intensity = s.a;
        }

    }

    // Remove fragments outside the clipping plane.
    if (dot(uvw_pick, uvw_pick) == 0)
        discard;

    // Clipping slice image.
    if (in_clip && clip_front) {
        out_color = texture(tex_colors, uvw_pick);

        // NOTE: if color alpha is zero, do not fetch from the clipping plane but use the
        // previously computed value from the volume
        if (out_color.a > .001) {
            acc = .25 * acc + .75 * out_color;
        }
    }

    out_color = acc;
    out_pick = ivec4(255 * uvw_pick, 0);
}
