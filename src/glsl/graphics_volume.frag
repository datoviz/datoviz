#version 450
#include "common.glsl"

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


bool intersect_box(vec3 origin, vec3 dir, vec3 box_min, vec3 box_max)
{
    vec3 inv_r = 1.0 / dir;
    vec3 tbot = inv_r * (box_min-origin);
    vec3 ttop = inv_r * (box_max-origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    float t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    float t1 = min(t.x, t.y);
    return t0 <= t1;
}


void main()
{
    // vec3 orig = params.view_pos;
    vec3 u = normalize(in_ray);
    vec3 c = vec3(0);
    vec3 o = params.view_pos.xyz;

    float r = .25;
    vec3 b0 = vec3(-r);
    vec3 b1 = vec3(r);
    mat4 m = inverse(mvp.model);
    bool b = intersect_box((m*vec4(o, 1)).xyz, (m*vec4(u, 1)).xyz, b0, b1);

    // float delta = pow(dot(u, o-c), 2) - (dot(o-c, o-c)-r*r);
    // float a = b ? .75 : .25;
    // if (delta < 0) discard;

    out_color = vec4(in_uvw, 1);
    out_color.xyz *= (b ? 1.25 : .75);
    // out_color.r = a;

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

    // Or computing directly in the shader. Limited to a few colormaps. Not sure which is faster.
    // out_color = colormap(params.cmap, value);

    // out_color.a = alpha;
}
