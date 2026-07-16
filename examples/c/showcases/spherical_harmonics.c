/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* spherical_harmonics - This example turns a real spherical-harmonic blend into a lit mesh.
 *
 * What to look for: a uniformly tessellated icosphere is deformed radially by a deterministic
 * blend of real spherical harmonics. The coefficients morph in a slow seamless loop, vertex color
 * preserves the signed amplitude, and smooth normals reveal the changing folded surface.
 *
 * This workflow is useful for directional basis functions, radiation patterns, orbital-like
 * surfaces, and other spherical scalar fields that benefit from direct 3D shape perception.
 *
 * Scenario: showcases_spherical_harmonics
 * Style: showcase, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c showcases/spherical_harmonics
 * Run:    ./build/examples/c/showcases/spherical_harmonics --live
 * Smoke:  ./build/examples/c/showcases/spherical_harmonics --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_alloc.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

#define ICOSPHERE_SUBDIVISIONS 5u
#define HARMONIC_TERM_COUNT    4u
#define HARMONIC_BASE_RADIUS   0.43
#define HARMONIC_LOBE_SCALE    1.04
#define HARMONIC_LOBE_POWER    0.78
#define HARMONIC_MORPH_PERIOD  8.0

#define PI 3.14159265358979323846



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct HarmonicTerm
{
    uint32_t degree;
    int32_t order;
    double base_weight;
    double morph_amplitude;
    uint32_t morph_frequency;
} HarmonicTerm;


typedef struct MeshBuffers
{
    dvec3* positions;
    DvzIndex* indices;
    uint32_t vertex_count;
    uint32_t index_count;
} MeshBuffers;


typedef struct EdgeEntry
{
    uint64_t key;
    DvzIndex midpoint;
    bool used;
} EdgeEntry;


typedef struct SphericalHarmonicsState
{
    DvzGeometry* geometry;
    DvzVisual* visual;
    dvec3* directions;
    double* basis;
    double* amplitudes;
    vec3* gpu_positions;
    vec3* gpu_normals;
} SphericalHarmonicsState;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static const HarmonicTerm HARMONIC_TERMS[HARMONIC_TERM_COUNT] = {
    {.degree = 7u,
     .order = +3,
     .base_weight = +1.00,
     .morph_amplitude = 0.22,
     .morph_frequency = 1u},
    {.degree = 6u,
     .order = -5,
     .base_weight = +0.58,
     .morph_amplitude = 0.25,
     .morph_frequency = 2u},
    {.degree = 5u,
     .order = +2,
     .base_weight = -0.38,
     .morph_amplitude = 0.20,
     .morph_frequency = 3u},
    {.degree = 4u,
     .order = -1,
     .base_weight = +0.25,
     .morph_amplitude = 0.18,
     .morph_frequency = 1u},
};



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_spherical_harmonics_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Release temporary mesh arrays and reset their counts.
 *
 * @param mesh mesh arrays to release
 */
static void _mesh_buffers_reset(MeshBuffers* mesh)
{
    if (mesh == NULL)
        return;
    dvz_free(mesh->positions);
    dvz_free(mesh->indices);
    mesh->positions = NULL;
    mesh->indices = NULL;
    mesh->vertex_count = 0;
    mesh->index_count = 0;
}



/**
 * Normalize one double-precision vector in place.
 *
 * @param v vector to normalize
 * @return whether the vector had a finite non-zero norm
 */
static bool _normalize(dvec3 v)
{
    const double norm = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (!(norm > 0.0) || !isfinite(norm))
        return false;
    v[0] /= norm;
    v[1] /= norm;
    v[2] /= norm;
    return true;
}



/**
 * Return the next power of two greater than or equal to a value.
 *
 * @param value positive input value
 * @return power-of-two capacity, or zero on overflow
 */
static uint32_t _next_power_of_two(uint32_t value)
{
    if (value == 0)
        return 1;
    value--;
    value |= value >> 1u;
    value |= value >> 2u;
    value |= value >> 4u;
    value |= value >> 8u;
    value |= value >> 16u;
    value++;
    return value;
}



/**
 * Return a stable undirected edge key for two vertex indices.
 *
 * @param a first vertex index
 * @param b second vertex index
 * @return packed edge key
 */
static uint64_t _edge_key(DvzIndex a, DvzIndex b)
{
    const uint32_t lo = a < b ? a : b;
    const uint32_t hi = a < b ? b : a;
    return ((uint64_t)lo << 32u) | (uint64_t)hi;
}



/**
 * Find or create the normalized midpoint shared by one mesh edge.
 *
 * @param positions mutable vertex array
 * @param vertex_count mutable number of populated vertices
 * @param vertex_capacity allocated vertex capacity
 * @param cache edge midpoint cache
 * @param cache_capacity power-of-two cache capacity
 * @param a first edge endpoint
 * @param b second edge endpoint
 * @param out output midpoint vertex index
 * @return whether the midpoint was found or created
 */
static bool _edge_midpoint(
    dvec3* positions, uint32_t* vertex_count, uint32_t vertex_capacity, EdgeEntry* cache,
    uint32_t cache_capacity, DvzIndex a, DvzIndex b, DvzIndex* out)
{
    if (positions == NULL || vertex_count == NULL || cache == NULL || cache_capacity == 0 ||
        out == NULL || a >= *vertex_count || b >= *vertex_count)
    {
        return false;
    }

    const uint64_t key = _edge_key(a, b);
    uint32_t slot = (uint32_t)((key * UINT64_C(11400714819323198485)) & (cache_capacity - 1u));
    while (cache[slot].used)
    {
        if (cache[slot].key == key)
        {
            *out = cache[slot].midpoint;
            return true;
        }
        slot = (slot + 1u) & (cache_capacity - 1u);
    }

    if (*vertex_count >= vertex_capacity)
        return false;

    const uint32_t midpoint = *vertex_count;
    positions[midpoint][0] = 0.5 * (positions[a][0] + positions[b][0]);
    positions[midpoint][1] = 0.5 * (positions[a][1] + positions[b][1]);
    positions[midpoint][2] = 0.5 * (positions[a][2] + positions[b][2]);
    if (!_normalize(positions[midpoint]))
        return false;
    *vertex_count = midpoint + 1u;

    cache[slot].used = true;
    cache[slot].key = key;
    cache[slot].midpoint = midpoint;
    *out = midpoint;
    return true;
}



/**
 * Initialize a unit icosahedron with shared vertices.
 *
 * @param out output mesh arrays
 * @return whether allocation and initialization succeeded
 */
static bool _icosahedron(MeshBuffers* out)
{
    if (out == NULL)
        return false;

    static const double vertices[12][3] = {
        {-1.0, +1.618033988749895, 0.0}, {+1.0, +1.618033988749895, 0.0},
        {-1.0, -1.618033988749895, 0.0}, {+1.0, -1.618033988749895, 0.0},
        {0.0, -1.0, +1.618033988749895}, {0.0, +1.0, +1.618033988749895},
        {0.0, -1.0, -1.618033988749895}, {0.0, +1.0, -1.618033988749895},
        {+1.618033988749895, 0.0, -1.0}, {+1.618033988749895, 0.0, +1.0},
        {-1.618033988749895, 0.0, -1.0}, {-1.618033988749895, 0.0, +1.0},
    };
    static const DvzIndex faces[60] = {
        0, 11, 5,  0, 5,  1, 0, 1, 7, 0, 7,  10, 0, 10, 11, 1, 5, 9, 5, 11,
        4, 11, 10, 2, 10, 7, 6, 7, 1, 8, 3,  9,  4, 3,  4,  2, 3, 2, 6, 3,
        6, 8,  3,  8, 9,  4, 9, 5, 2, 4, 11, 6,  2, 10, 8,  6, 7, 9, 8, 1,
    };

    out->positions = (dvec3*)dvz_calloc(12u, sizeof(dvec3));
    out->indices = (DvzIndex*)dvz_calloc(60u, sizeof(DvzIndex));
    if (out->positions == NULL || out->indices == NULL)
    {
        _mesh_buffers_reset(out);
        return false;
    }

    out->vertex_count = 12u;
    out->index_count = 60u;
    for (uint32_t i = 0; i < out->vertex_count; i++)
    {
        out->positions[i][0] = vertices[i][0];
        out->positions[i][1] = vertices[i][1];
        out->positions[i][2] = vertices[i][2];
        if (!_normalize(out->positions[i]))
        {
            _mesh_buffers_reset(out);
            return false;
        }
    }
    (void)dvz_memcpy(out->indices, 60u * sizeof(DvzIndex), faces, sizeof(faces));
    return true;
}



/**
 * Subdivide every triangle into four triangles while sharing edge midpoints.
 *
 * @param mesh input and output mesh arrays
 * @return whether subdivision succeeded
 */
static bool _subdivide(MeshBuffers* mesh)
{
    if (mesh == NULL || mesh->positions == NULL || mesh->indices == NULL ||
        mesh->index_count == 0 || mesh->index_count % 3u != 0)
    {
        return false;
    }

    const uint32_t face_count = mesh->index_count / 3u;
    if (face_count > UINT32_MAX / 12u || mesh->vertex_count > UINT32_MAX - 3u * face_count)
        return false;
    const uint32_t vertex_capacity = mesh->vertex_count + 3u * face_count;
    const uint32_t next_index_count = mesh->index_count * 4u;
    const uint32_t cache_capacity = _next_power_of_two(4u * face_count);
    if (cache_capacity == 0)
        return false;

    MeshBuffers next = {0};
    next.positions = (dvec3*)dvz_calloc(vertex_capacity, sizeof(dvec3));
    next.indices = (DvzIndex*)dvz_calloc(next_index_count, sizeof(DvzIndex));
    EdgeEntry* cache = (EdgeEntry*)dvz_calloc(cache_capacity, sizeof(EdgeEntry));
    if (next.positions == NULL || next.indices == NULL || cache == NULL)
        goto error;

    const size_t position_bytes = (size_t)mesh->vertex_count * sizeof(dvec3);
    (void)dvz_memcpy(
        next.positions, (size_t)vertex_capacity * sizeof(dvec3), mesh->positions, position_bytes);
    next.vertex_count = mesh->vertex_count;
    next.index_count = next_index_count;

    for (uint32_t face = 0; face < face_count; face++)
    {
        const DvzIndex a = mesh->indices[3u * face + 0u];
        const DvzIndex b = mesh->indices[3u * face + 1u];
        const DvzIndex c = mesh->indices[3u * face + 2u];
        DvzIndex ab = 0, bc = 0, ca = 0;
        if (!_edge_midpoint(
                next.positions, &next.vertex_count, vertex_capacity, cache, cache_capacity, a, b,
                &ab) ||
            !_edge_midpoint(
                next.positions, &next.vertex_count, vertex_capacity, cache, cache_capacity, b, c,
                &bc) ||
            !_edge_midpoint(
                next.positions, &next.vertex_count, vertex_capacity, cache, cache_capacity, c, a,
                &ca))
        {
            goto error;
        }

        const uint32_t i = 12u * face;
        next.indices[i + 0u] = a;
        next.indices[i + 1u] = ab;
        next.indices[i + 2u] = ca;
        next.indices[i + 3u] = b;
        next.indices[i + 4u] = bc;
        next.indices[i + 5u] = ab;
        next.indices[i + 6u] = c;
        next.indices[i + 7u] = ca;
        next.indices[i + 8u] = bc;
        next.indices[i + 9u] = ab;
        next.indices[i + 10u] = bc;
        next.indices[i + 11u] = ca;
    }

    dvz_free(cache);
    _mesh_buffers_reset(mesh);
    *mesh = next;
    return true;

error:
    dvz_free(cache);
    _mesh_buffers_reset(&next);
    return false;
}



/**
 * Evaluate the associated Legendre polynomial P_l^m at x.
 *
 * @param degree polynomial degree
 * @param order non-negative polynomial order
 * @param x input coordinate in [-1, +1]
 * @return associated Legendre value
 */
static double _associated_legendre(uint32_t degree, uint32_t order, double x)
{
    if (order > degree)
        return 0.0;

    double p_mm = 1.0;
    if (order > 0)
    {
        const double root = sqrt(fmax(0.0, 1.0 - x * x));
        double factor = 1.0;
        for (uint32_t i = 1; i <= order; i++)
        {
            p_mm *= -factor * root;
            factor += 2.0;
        }
    }
    if (degree == order)
        return p_mm;

    double p_lm_prev = x * (2.0 * (double)order + 1.0) * p_mm;
    if (degree == order + 1u)
        return p_lm_prev;

    double p_lm_prev2 = p_mm;
    for (uint32_t l = order + 2u; l <= degree; l++)
    {
        const double p_lm = ((2.0 * (double)l - 1.0) * x * p_lm_prev -
                             ((double)l + (double)order - 1.0) * p_lm_prev2) /
                            ((double)l - (double)order);
        p_lm_prev2 = p_lm_prev;
        p_lm_prev = p_lm;
    }
    return p_lm_prev;
}



/**
 * Evaluate one orthonormal real spherical harmonic.
 *
 * @param degree harmonic degree
 * @param order signed harmonic order
 * @param cos_theta cosine of the polar angle
 * @param phi azimuthal angle in radians
 * @return real spherical-harmonic value
 */
static double
_real_spherical_harmonic(uint32_t degree, int32_t order, double cos_theta, double phi)
{
    const uint32_t abs_order = (uint32_t)(order < 0 ? -order : order);
    if (abs_order > degree)
        return 0.0;

    const double log_ratio =
        lgamma((double)(degree - abs_order + 1u)) - lgamma((double)(degree + abs_order + 1u));
    const double normalization =
        sqrt(((2.0 * (double)degree + 1.0) / (4.0 * PI)) * exp(log_ratio));
    const double p = _associated_legendre(degree, abs_order, cos_theta);
    if (order > 0)
        return sqrt(2.0) * normalization * cos((double)abs_order * phi) * p;
    if (order < 0)
        return sqrt(2.0) * normalization * sin((double)abs_order * phi) * p;
    return normalization * p;
}



/**
 * Evaluate every harmonic basis term at one unit direction.
 *
 * @param direction unit sphere direction
 * @param out output basis values
 */
static void _harmonic_basis(const dvec3 direction, double out[HARMONIC_TERM_COUNT])
{
    const double cos_theta = fmax(-1.0, fmin(+1.0, direction[2]));
    const double phi = atan2(direction[1], direction[0]);
    for (uint32_t term = 0; term < HARMONIC_TERM_COUNT; term++)
    {
        out[term] = _real_spherical_harmonic(
            HARMONIC_TERMS[term].degree, HARMONIC_TERMS[term].order, cos_theta, phi);
    }
}



/**
 * Compute the periodic harmonic coefficients for one animation phase.
 *
 * @param phase seamless animation phase in [0, 1)
 * @param out output harmonic weights
 */
static void _harmonic_weights(double phase, double out[HARMONIC_TERM_COUNT])
{
    for (uint32_t term = 0; term < HARMONIC_TERM_COUNT; term++)
    {
        const double angle = 2.0 * PI * (double)HARMONIC_TERMS[term].morph_frequency * phase;
        out[term] =
            HARMONIC_TERMS[term].base_weight + HARMONIC_TERMS[term].morph_amplitude * sin(angle);
    }
}



/**
 * Linearly interpolate between two colors.
 *
 * @param a first color
 * @param b second color
 * @param t interpolation coordinate in [0, 1]
 * @return interpolated opaque color
 */
static DvzColor _mix_color(DvzColor a, DvzColor b, double t)
{
    t = fmax(0.0, fmin(1.0, t));
    return dvz_color_rgba(
        (uint8_t)lround((1.0 - t) * (double)a.r + t * (double)b.r),
        (uint8_t)lround((1.0 - t) * (double)a.g + t * (double)b.g),
        (uint8_t)lround((1.0 - t) * (double)a.b + t * (double)b.b), 255u);
}



/**
 * Map a normalized signed harmonic amplitude to the showcase palette.
 *
 * @param amplitude signed amplitude in [-1, +1]
 * @return opaque vertex color
 */
static DvzColor _harmonic_color(double amplitude)
{
    static const DvzColor stops[5] = {
        {.r = 50, .g = 39, .b = 112, .a = 255},   {.r = 27, .g = 158, .b = 181, .a = 255},
        {.r = 225, .g = 229, .b = 215, .a = 255}, {.r = 246, .g = 169, .b = 79, .a = 255},
        {.r = 225, .g = 70, .b = 79, .a = 255},
    };

    const double x = 2.0 * (0.5 * (fmax(-1.0, fmin(+1.0, amplitude)) + 1.0));
    const uint32_t segment = x >= 2.0 ? 3u : (uint32_t)floor(x * 2.0);
    const double local = x >= 2.0 ? 1.0 : x * 2.0 - (double)segment;
    return _mix_color(stops[segment], stops[segment + 1u], local);
}



/**
 * Release all CPU-side animation arrays owned by the showcase state.
 *
 * @param state state to reset
 */
static void _state_reset(SphericalHarmonicsState* state)
{
    if (state == NULL)
        return;
    dvz_geometry_destroy(state->geometry);
    dvz_free(state->directions);
    dvz_free(state->basis);
    dvz_free(state->amplitudes);
    dvz_free(state->gpu_positions);
    dvz_free(state->gpu_normals);
    state->geometry = NULL;
    state->visual = NULL;
    state->directions = NULL;
    state->basis = NULL;
    state->amplitudes = NULL;
    state->gpu_positions = NULL;
    state->gpu_normals = NULL;
}



/**
 * Update positions, colors, and normals for one animation phase.
 *
 * @param state initialized showcase state
 * @param phase seamless animation phase in [0, 1)
 * @return whether the geometry update succeeded
 */
static bool _update_geometry(SphericalHarmonicsState* state, double phase)
{
    if (state == NULL || state->geometry == NULL || state->directions == NULL ||
        state->basis == NULL || state->amplitudes == NULL || state->gpu_positions == NULL ||
        state->gpu_normals == NULL)
    {
        return false;
    }

    double weights[HARMONIC_TERM_COUNT] = {0};
    _harmonic_weights(phase, weights);

    double max_amplitude = 0.0;
    for (uint32_t vertex = 0; vertex < state->geometry->vertex_count; vertex++)
    {
        double amplitude = 0.0;
        for (uint32_t term = 0; term < HARMONIC_TERM_COUNT; term++)
        {
            amplitude += weights[term] * state->basis[vertex * HARMONIC_TERM_COUNT + term];
        }
        state->amplitudes[vertex] = amplitude;
        max_amplitude = fmax(max_amplitude, fabs(amplitude));
    }
    if (!(max_amplitude > 0.0) || !isfinite(max_amplitude))
        return false;

    const double breathing = 0.96 + 0.04 * cos(2.0 * PI * phase);
    for (uint32_t vertex = 0; vertex < state->geometry->vertex_count; vertex++)
    {
        const double normalized = state->amplitudes[vertex] / max_amplitude;
        const double radius =
            HARMONIC_BASE_RADIUS +
            HARMONIC_LOBE_SCALE * breathing * pow(fabs(normalized), HARMONIC_LOBE_POWER);
        for (uint32_t axis = 0; axis < 3u; axis++)
        {
            state->geometry->positions[vertex][axis] = radius * state->directions[vertex][axis];
        }
        state->geometry->colors[vertex] = _harmonic_color(normalized);
    }
    if (dvz_geometry_compute_normals(state->geometry) != DVZ_OK)
        return false;

    for (uint32_t vertex = 0; vertex < state->geometry->vertex_count; vertex++)
    {
        for (uint32_t axis = 0; axis < 3u; axis++)
        {
            state->gpu_positions[vertex][axis] = (float)state->geometry->positions[vertex][axis];
            state->gpu_normals[vertex][axis] = (float)state->geometry->normals[vertex][axis];
        }
    }
    return true;
}



/**
 * Generate the indexed icosphere and cache its harmonic basis values.
 *
 * @param state output showcase state
 * @return whether geometry generation and initial deformation succeeded
 */
static bool _create_geometry(SphericalHarmonicsState* state)
{
    if (state == NULL)
        return false;

    bool ok = false;
    MeshBuffers mesh = {0};
    if (!_icosahedron(&mesh))
        goto cleanup;
    for (uint32_t level = 0; level < ICOSPHERE_SUBDIVISIONS; level++)
    {
        if (!_subdivide(&mesh))
            goto cleanup;
    }

    state->geometry = dvz_geometry(mesh.vertex_count, mesh.index_count);
    state->directions = (dvec3*)dvz_calloc(mesh.vertex_count, sizeof(dvec3));
    state->basis =
        (double*)dvz_calloc((size_t)mesh.vertex_count * HARMONIC_TERM_COUNT, sizeof(double));
    state->amplitudes = (double*)dvz_calloc(mesh.vertex_count, sizeof(double));
    state->gpu_positions = (vec3*)dvz_calloc(mesh.vertex_count, sizeof(vec3));
    state->gpu_normals = (vec3*)dvz_calloc(mesh.vertex_count, sizeof(vec3));
    if (state->geometry == NULL || state->directions == NULL || state->basis == NULL ||
        state->amplitudes == NULL || state->gpu_positions == NULL || state->gpu_normals == NULL)
    {
        goto cleanup;
    }

    const size_t direction_bytes = (size_t)mesh.vertex_count * sizeof(dvec3);
    const size_t index_bytes = (size_t)mesh.index_count * sizeof(DvzIndex);
    (void)dvz_memcpy(state->directions, direction_bytes, mesh.positions, direction_bytes);
    (void)dvz_memcpy(state->geometry->indices, index_bytes, mesh.indices, index_bytes);
    for (uint32_t vertex = 0; vertex < mesh.vertex_count; vertex++)
    {
        _harmonic_basis(state->directions[vertex], &state->basis[vertex * HARMONIC_TERM_COUNT]);
    }
    ok = _update_geometry(state, 0.0);

cleanup:
    _mesh_buffers_reset(&mesh);
    if (!ok)
        _state_reset(state);
    return ok;
}



/**
 * Upload the current dynamic mesh attributes to the retained visual.
 *
 * @param state initialized showcase state
 * @return whether the retained update succeeded
 */
static bool _upload_geometry(SphericalHarmonicsState* state)
{
    if (state == NULL || state->visual == NULL || state->geometry == NULL)
        return false;
    const uint32_t vertex_count = state->geometry->vertex_count;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = state->gpu_positions, .item_count = vertex_count},
        {.attr_name = "color", .data = state->geometry->colors, .item_count = vertex_count},
        {.attr_name = "normal", .data = state->gpu_normals, .item_count = vertex_count},
    };
    return dvz_visual_set_data_many(state->visual, updates, DVZ_ARRAY_COUNT(updates)) == DVZ_OK;
}



/**
 * Resolve the animation phase for live and deterministic preview execution.
 *
 * @param ctx scenario context
 * @return seamless phase in [0, 1)
 */
static double _animation_phase(const DvzScenarioContext* ctx)
{
    if (ctx == NULL)
        return 0.0;
    if (ctx->preview_mode)
        return dvz_scenario_preview_phase(ctx, DVZ_SCENARIO_PREVIEW_PHASE_SEAMLESS_LOOP);
    const double time = fmax(0.0, ctx->time);
    return fmod(time / HARMONIC_MORPH_PERIOD, 1.0);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the spherical-harmonics showcase.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    bool ok = false;
    SphericalHarmonicsState* state =
        (SphericalHarmonicsState*)dvz_calloc(1, sizeof(SphericalHarmonicsState));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;

    EXAMPLE_CHECK(_create_geometry(state), "spherical-harmonics geometry generation failed");

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera = example_default_3d_camera_desc(1.35f);
    camera.view.eye[0] = -0.46f;
    camera.view.eye[1] = +2.20f;
    camera.view.eye[2] = +3.42f;
    camera.projection.fov_y = 0.58f;
    const int camera_result = dvz_panel_set_camera_desc(panel, &camera);
    EXAMPLE_CHECK(camera_result == DVZ_OK, "dvz_panel_set_camera_desc() failed");

    state->visual = dvz_mesh(ctx->scene, 0);
    EXAMPLE_CHECK(state->visual != NULL, "dvz_mesh() failed");
    const int geometry_result = dvz_mesh_set_geometry(state->visual, state->geometry);
    EXAMPLE_CHECK(geometry_result == DVZ_OK, "dvz_mesh_set_geometry() failed");

    DvzMaterialDesc material = example_default_standard_material_desc();
    material.light_direction[0] = -0.48f;
    material.light_direction[1] = +0.62f;
    material.light_direction[2] = +0.72f;
    material.standard.roughness = 0.44f;
    material.standard.specular = 0.28f;
    material.standard.rim_strength = 0.18f;
    const int material_result = dvz_visual_set_material(state->visual, &material);
    EXAMPLE_CHECK(material_result == DVZ_OK, "dvz_visual_set_material() failed");

    const int add_result = dvz_panel_add_visual(panel, state->visual, NULL);
    EXAMPLE_CHECK(add_result == DVZ_OK, "dvz_panel_add_visual() failed");
    const int primary_result = dvz_scenario_set_primary_visual(ctx, state->visual);
    EXAMPLE_CHECK(primary_result == DVZ_OK, "dvz_scenario_set_primary_visual() failed");

#ifndef DVZ_EXAMPLE_NO_APP
    DvzMsaaDesc msaa = dvz_msaa_desc();
    msaa.sample_count = 8u;
    const int msaa_result = dvz_panel_set_msaa(panel, &msaa);
    EXAMPLE_CHECK(msaa_result == DVZ_OK, "dvz_panel_set_msaa() failed");
#endif

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    EXAMPLE_CHECK(controller != NULL, "dvz_arcball() failed");
    DvzArcball* arcball = dvz_controller_arcball(controller);
    EXAMPLE_CHECK(arcball != NULL, "dvz_controller_arcball() failed");
    const int bind_result = dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ);
    EXAMPLE_CHECK(bind_result == DVZ_OK, "dvz_scenario_bind_controller() failed");
    vec3 initial_angles = {-0.32f, +0.48f, -0.16f};
    dvz_arcball_initial(arcball, initial_angles);

    ok = true;
cleanup:
    return ok;
}



/**
 * Advance the seamless harmonic morph and upload its retained attributes.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    SphericalHarmonicsState* state = (SphericalHarmonicsState*)user;
    if (ctx == NULL || state == NULL)
        return;
    const double phase = _animation_phase(ctx);
    if (!_update_geometry(state, phase))
        return;
    (void)_upload_geometry(state);
}



/**
 * Destroy the spherical-harmonics showcase state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    SphericalHarmonicsState* state = (SphericalHarmonicsState*)user;
    if (state == NULL)
        return;
    _state_reset(state);
    dvz_free(state);
}



/**
 * Return the spherical-harmonics showcase scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_showcase_spherical_harmonics_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "showcases_spherical_harmonics",
        .title = "Spherical Harmonics",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_ARCBALL | DVZ_SCENARIO_REQ_FRAME_CALLBACKS |
                        DVZ_SCENARIO_REQ_CONTINUOUS_FRAMES,
        .continuous_frames = true,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the spherical-harmonics showcase through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_spherical_harmonics_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == DVZ_OK ? 0 : 1;
}
#endif
