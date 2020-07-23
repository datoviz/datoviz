#version 450
#include "common.glsl"

#define STEP_SIZE 0.0025
#define MAX_ITER 10 / STEP_SIZE

#define VKY_VOLUME_MIP 0
#define VKY_VOLUME_BLEND 1
#define VKY_VOLUME_ISOSURFACE 2

layout (binding = 2) uniform VolumeParams {
    mat4 inv_proj_view;
    mat4 normal_mat;
} params;

layout (binding = 3) uniform sampler3D volume;

layout (location = 0) in vec2 tex_coords;

layout (location = 0) out vec4 out_color;


const uint render_type = VKY_VOLUME_MIP;
const float gamma = 1.0;
const vec3 light_position = vec3(-.5, -.5, 1);
const vec3 material_colour = vec3(1, 1, 1);
const vec3 background_color = vec3(0, 0, 0);


struct Ray {
    vec3 origin;
    vec3 dir;
};


float fetch(vec3 pos) {
    return texture(volume, vec3(pos.x, 1-pos.y, 1-pos.z)).r;
}


bool intersect_box(Ray r, vec3 box_min, vec3 box_max, out float t0, out float t1)
{
    vec3 inv_r = 1.0 / r.dir;
    vec3 tbot = inv_r * (box_min-r.origin);
    vec3 ttop = inv_r * (box_max-r.origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);
    return t0 <= t1;
}


// A very simple color transfer function
vec4 color_transfer(float intensity)
{
    vec3 high = vec3(1.0, 1.0, 1.0);
    vec3 low = vec3(0.0, 0.0, 0.0);
    float alpha = (exp(intensity) - 1.0) / (exp(1.0) - 1.0);
    return vec4(intensity * high + (1.0 - intensity) * low, alpha);
}


// Estimate normal from a finite difference approximation of the gradient
vec3 normal(vec3 position, float intensity)
{
    float d = STEP_SIZE;
    float dx = fetch(position + vec3(d,0,0)) - intensity;
    float dy = fetch(position + vec3(0,d,0)) - intensity;
    float dz = fetch(position + vec3(0,0,d)) - intensity;
    return -normalize(mat3(params.normal_mat) * vec3(dx, dy, dz));
}


void main()
{
    vec2 p = 2.0 * gl_FragCoord.xy / mvp.viewport.zw - 1.0;

    // HACK: threshold obtained from the mouse zoom.
    float threshold = clamp(-mvp.view[3][2] / 10, .1, .9);

    mat4 mat = params.inv_proj_view;
    vec3 p0 = (mat * vec4(p, -1, 1)).xyz;
    vec3 p1 = (mat * vec4(p, +1, 1)).xyz;
    vec3 dir = normalize(p1 - p0);

    Ray ray = Ray(p0, dir);
    float t0, t1;
    if (!intersect_box(ray, vec3(-0.25), vec3(+0.25), t0, t1)) discard;
    if (t0 < 0 || t1 < 0) discard;

    vec3 ray_start = p0 + dir * t0;
    vec3 ray_stop = p0 + dir * t1;

    if (render_type == VKY_VOLUME_BLEND) {
        // NOTE: inverse for blend.
        ray_start = p0 + dir * t1;
        ray_stop = p0 + dir * t0;
    }

    // Perform the ray marching:
    vec3 pos = ray_start;
    vec3 step = normalize(ray_stop - ray_start) * STEP_SIZE;
    float travel = distance(ray_stop, ray_start);
    float maximum_intensity = 0.0;

    out_color = vec4(0, 0, 0, 1);
    vec4 color = vec4(0.0);
    color.a = 1.0;

    for (int i = 0; i < MAX_ITER && travel > 0.0; ++i, pos += step, travel -= STEP_SIZE) {
        vec3 uvw = (1 + 4 * pos) / 2;
        // Transposition.
        uvw.xyz = uvw.zxy;
        float intensity = fetch(uvw);

        // MIP
        if (intensity > maximum_intensity) {
            maximum_intensity = intensity;
        }

        // Alpha-blending
        if (render_type == VKY_VOLUME_BLEND) {
            vec4 c = color_transfer(intensity);
            color.rgb = c.a * c.rgb + (1 - c.a) * color.a * color.rgb;
            color.a = c.a + (1 - c.a) * color.a;
        }

        // Isosurface.
        if (render_type == VKY_VOLUME_ISOSURFACE && intensity > threshold) {

            // Get closer to the surface
            uvw -= step * 0.5;
            intensity = fetch(uvw);
            uvw -= step * (intensity > threshold ? 0.25 : -0.25);
            intensity = fetch(uvw);

            // Blinn-Phong shading
            vec3 L = normalize(light_position - uvw);
            vec3 V = -normalize(step);
            vec3 N = normal(uvw, intensity);
            vec3 H = normalize(L + V);

            float Ia = 0.1;
            float Id = 1.0 * max(0, dot(N, L));
            float Is = 8.0 * pow(max(0, dot(N, H)), 600);
            color.rgb = (Ia + Id) * material_colour + Is * vec3(1.0);

            break;
        }
    }

    switch (render_type) {
        case VKY_VOLUME_MIP:
            // MIP
            color.rgb = vec3(maximum_intensity);
            break;
        case VKY_VOLUME_BLEND:
            // Blend
            color.rgb = color.a * color.rgb + (1 - color.a) * pow(background_color.xyz, vec3(gamma));
            break;
        case VKY_VOLUME_ISOSURFACE:
            break;
    }

    // Gamma correction
    out_color.rgb = pow(color.rgb, vec3(1.0 / gamma));
    out_color.a = color.a;
}
