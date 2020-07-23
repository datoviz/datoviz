#version 450
#include "../../src/shaders/common.glsl"

/*
Reference : vispy "raytracing.py" example
*/

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_coords;

layout (location = 0) out vec4 out_color;

const float M_PI = 3.14159265358979323846;
const float INFINITY = 1000000000.;
const int PLANE = 1;
const int SPHERE_0 = 2;
const int SPHERE_1 = 3;

float u_aspect_ratio = mvp.viewport.z / mvp.viewport.w;  // TODO

const vec3 sphere_position_0 = vec3(.75, .1, -2.);
const float sphere_radius_0 = .6;
const vec3 sphere_color_0 = vec3(0., 0., 1.);

const vec3 sphere_position_1 = vec3(-.75, .1, -2.25);
const float sphere_radius_1 = .6;
const vec3 sphere_color_1 = vec3(.5, .223, .5);

const vec3 plane_position = vec3(0., -.5, 0.);
const vec3 plane_normal = vec3(0., 1., 0.);

const float light_intensity = 1;
const vec2 light_specular = vec2(1., 50.);
const vec3 light_position = vec3(5., 5., 0.);
const vec3 light_color = vec3(1., 1., 1.);

const float ambient = .05;

float intersect_sphere(vec3 O, vec3 D, vec3 S, float R) {
    float a = dot(D, D);
    vec3 OS = O - S;
    float b = 2. * dot(D, OS);
    float c = dot(OS, OS) - R * R;
    float disc = b * b - 4. * a * c;
    if (disc > 0.) {
        float distSqrt = sqrt(disc);
        float q = (-b - distSqrt) / 2.0;
        if (b >= 0.) {
            q = (-b + distSqrt) / 2.0;
        }
        float t0 = q / a;
        float t1 = c / q;
        t0 = min(t0, t1);
        t1 = max(t0, t1);
        if (t1 >= 0.) {
            if (t0 < 0.) {
                return t1;
            }
            else {
                return t0;
            }
        }
    }
    return INFINITY;
}

float intersect_plane(vec3 O, vec3 D, vec3 P, vec3 N) {
    float denom = dot(D, N);
    if (abs(denom) < 1e-6) {
        return INFINITY;
    }
    float d = dot(P - O, N) / denom;
    if (d < 0.) {
        return INFINITY;
    }
    return d;
}

void main() {
    mat4 view = inverse(mvp.view);
    vec3 right = view[0].xyz;
    vec3 up = view[1].xyz;
    vec3 forward = 2 * view[2].xyz;  // coefficient depending on the field of view
    vec3 eye_pos = view[3].xyz;

    vec3 D = normalize(u_aspect_ratio * in_pos.x * right + in_pos.y * up - forward);
    vec3 O = eye_pos;

    int depth = 0;
    float t_plane, t0, t1;
    vec3 rayO = O;
    vec3 rayD = D;
    vec3 col = vec3(0.0, 0.0, 0.0);
    vec3 col_ray;
    float reflection = 1.;

    int object_index;
    vec3 object_color;
    vec3 object_normal;
    float object_reflection;
    vec3 M;
    vec3 N, toL, toO;

    while (depth < 5) {

        /* start trace_ray */

        t_plane = intersect_plane(rayO, rayD, plane_position, plane_normal);
        t0 = intersect_sphere(rayO, rayD, sphere_position_0, sphere_radius_0);
        t1 = intersect_sphere(rayO, rayD, sphere_position_1, sphere_radius_1);

        if (t_plane < min(t0, t1)) {
            // Plane.
            M = rayO + rayD * t_plane;
            object_normal = plane_normal;
            // Plane texture.
            if (mod(int(round(2*M.x)), 2) == mod(int(round(2*M.z)), 2)) {
                object_color = vec3(1., 1., 1.);
            }
            else {
                object_color = vec3(0., 0., 0.);
            }
            object_reflection = .25;
            object_index = PLANE;
        }
        else if (t0 < t1) {
            // Sphere 0.
            M = rayO + rayD * t0;
            object_normal = normalize(M - sphere_position_0);
            object_color = sphere_color_0;
            object_reflection = .5;
            object_index = SPHERE_0;
        }
        else if (t1 < t0) {
            // Sphere 1.
            M = rayO + rayD * t1;
            object_normal = normalize(M - sphere_position_1);
            object_color = sphere_color_1;
            object_reflection = .5;
            object_index = SPHERE_1;
        }
        else {
            break;
        }

        N = object_normal;
        // NOTE: the model matrix is used to move the light position here.
        toL = normalize((mvp.model * vec4(light_position, 1.0)).xyz - M);
        toO = normalize(O - M);

        // Shadow of the spheres on the plane.
        if (object_index == PLANE) {
            t0 = intersect_sphere(M + N * .0001, toL,
                                  sphere_position_0, sphere_radius_0);
            t1 = intersect_sphere(M + N * .0001, toL,
                                  sphere_position_1, sphere_radius_1);
            if (min(t0, t1) < INFINITY) {
                break;
            }
        }

        col_ray = vec3(ambient, ambient, ambient);
        col_ray += light_intensity * max(dot(N, toL), 0.) * object_color;
        col_ray += light_specular.x * light_color *
            pow(max(dot(N, normalize(toL + toO)), 0.), light_specular.y);

        /* end trace_ray */

        rayO = M + N * .0001;
        rayD = normalize(rayD - 2. * dot(rayD, N) * N);
        col += reflection * col_ray;
        reflection *= object_reflection;

        depth++;
    }

    out_color = vec4(clamp(col, 0., 1.), 1);

}
