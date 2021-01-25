#version 450
#include "common.glsl"

#define STEP_SIZE 0.0025
#define MAX_ITER 10 / STEP_SIZE

layout(std140, binding = USER_BINDING) uniform Params
{
    vec4 view_pos;
    int cmap;
}
params;

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex_cmap; // colormap texture
layout(binding = (USER_BINDING + 2)) uniform sampler3D tex;      // 3D volume

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_uvw;
layout(location = 2) in vec3 in_ray;

layout(location = 0) out vec4 out_color;


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


void main()
{
    mat4 mi = inverse(mvp.model);
    vec3 u = (mi * vec4(normalize(in_ray), 1)).xyz;
    vec3 o = (mi * vec4(params.view_pos.xyz, 1)).xyz;

    // // Inner cube example.
    // float r = .25;
    // vec3 b0 = vec3(-r);
    // vec3 b1 = vec3(+r);
    // bool b = intersect_box(o, u, b0, b1);
    // float a = b ? .75 : .25;
    // out_color = vec4(in_uvw, 1);
    // out_color.xyz *= a;
    // // Inner sphere example.
    // // float delta = pow(dot(u, o-c), 2) - (dot(o-c, o-c)-r*r);

    float t0, t1;
    float r = .75;
    vec3 b0 = vec3(-r);
    vec3 b1 = vec3(+r);
    intersect_box(o, u, b0, b1, t0, t1);
    if (t0 < 0 || t1 < 0) discard;

    vec3 ray_start = o + u * t0;
    vec3 ray_stop = o + u * t1;
    // out_color.xyz = ray_stop;
    // out_color.a = 1;

    vec3 pos = ray_start;
    vec3 step = normalize(ray_stop - ray_start) * STEP_SIZE;
    float travel = distance(ray_stop, ray_start);
    float max_intensity = 0.0;

    // vec4 color = vec4(0.0);
    // color.a = 1.0;
    vec3 uvw = vec3(0);
    for (int i = 0; i < MAX_ITER && travel > 0.0; ++i, pos += step, travel -= STEP_SIZE) {
        uvw = (pos - b0) / (b1 - b0);
        float intensity = texture(tex, uvw).r;
        // intensity *= exp(-pow(dot(pos, pos) / .25, 2));

        // MIP
        if (intensity > max_intensity) {
            max_intensity = intensity;
        }
    }
    if (max_intensity < .1) discard;
    out_color = texture(tex_cmap, vec2(max_intensity, (params.cmap + .5) / 256.0));

    // Fetch the value from the texture.
    // float value = texture(tex, in_uvw).r;

    // // Transfer function on the texture value.
    // if (sum(params.x_cmap) != 0)
    //     value = transfer(value, params.x_cmap, params.y_cmap);

    // // Transfer function on the texture value.
    // float alpha = 1.0;
    // if (sum(params.x_alpha) != 0)
    //     alpha = transfer(value, params.x_alpha, params.y_alpha);

    // // Sampling from the color texture.
    // out_color = texture(tex_cmap, vec2(value, (params.cmap + .5) / 256.0));
}
