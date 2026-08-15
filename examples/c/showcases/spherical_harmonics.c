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
 * Native live mode adds controls for the surface mapping and every harmonic term.
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
#include "example_tuner.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

#define ICOSPHERE_SUBDIVISIONS 5u
#define HARMONIC_TERM_COUNT    4u
#define HARMONIC_MORPH_PERIOD  8.0

#define PI 3.14159265358979323846



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct HarmonicTerm
{
    int degree;
    int order;
    float base_weight;
    float morph_amplitude;
    int morph_frequency;
    float morph_phase;
} HarmonicTerm;


typedef struct HarmonicSettings
{
    bool animate;
    float phase;
    float speed;
    float base_radius;
    float lobe_scale;
    float lobe_power;
    float node_smoothing;
    float breathing_amplitude;
    HarmonicTerm terms[HARMONIC_TERM_COUNT];
} HarmonicSettings;


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
    ExampleTuner tuner;
    DvzGeometry* geometry;
    DvzVisual* visual;
    DvzArcball* arcball;
    DvzMaterialDesc material;
    HarmonicSettings settings;
    HarmonicSettings defaults;
    dvec3* directions;
    double* basis;
    double* amplitudes;
    vec3* gpu_positions;
    vec3* gpu_normals;
    double last_time;
    bool time_initialized;
    bool basis_dirty;
    bool geometry_dirty;
    int preset_index;
    int basis_degree[HARMONIC_TERM_COUNT];
    int basis_order[HARMONIC_TERM_COUNT];
} SphericalHarmonicsState;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static const HarmonicSettings HARMONIC_DEFAULTS = {
    .animate = true,
    .phase = 0.0f,
    .speed = 1.0f,
    .base_radius = 0.54f,
    .lobe_scale = 0.94f,
    .lobe_power = 1.00f,
    .node_smoothing = 0.18f,
    .breathing_amplitude = 0.04f,
    .terms =
        {
            {.degree = 7,
             .order = +3,
             .base_weight = +1.00f,
             .morph_amplitude = 0.22f,
             .morph_frequency = 1,
             .morph_phase = 0.00f},
            {.degree = 6,
             .order = -5,
             .base_weight = +0.58f,
             .morph_amplitude = 0.25f,
             .morph_frequency = 2,
             .morph_phase = 0.00f},
            {.degree = 5,
             .order = +2,
             .base_weight = -0.38f,
             .morph_amplitude = 0.20f,
             .morph_frequency = 3,
             .morph_phase = 0.00f},
            {.degree = 4,
             .order = -1,
             .base_weight = +0.25f,
             .morph_amplitude = 0.18f,
             .morph_frequency = 1,
             .morph_phase = 0.00f},
        },
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
 * @param settings harmonic settings
 * @param out output basis values
 */
static void _harmonic_basis(
    const dvec3 direction, const HarmonicSettings* settings, double out[HARMONIC_TERM_COUNT])
{
    if (settings == NULL)
        return;
    const double cos_theta = fmax(-1.0, fmin(+1.0, direction[2]));
    const double phi = atan2(direction[1], direction[0]);
    for (uint32_t term = 0; term < HARMONIC_TERM_COUNT; term++)
    {
        out[term] = _real_spherical_harmonic(
            (uint32_t)settings->terms[term].degree, settings->terms[term].order, cos_theta, phi);
    }
}



/**
 * Compute the periodic harmonic coefficients for one animation phase.
 *
 * @param settings harmonic settings
 * @param phase seamless animation phase in [0, 1)
 * @param out output harmonic weights
 */
static void
_harmonic_weights(const HarmonicSettings* settings, double phase, double out[HARMONIC_TERM_COUNT])
{
    if (settings == NULL)
        return;
    for (uint32_t term = 0; term < HARMONIC_TERM_COUNT; term++)
    {
        const HarmonicTerm* item = &settings->terms[term];
        const double angle =
            2.0 * PI * ((double)item->morph_frequency * phase + (double)item->morph_phase);
        out[term] = (double)item->base_weight + (double)item->morph_amplitude * sin(angle);
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
    const DvzColor panel = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_PANEL_BG);
    const DvzColor stops[5] = {
        _mix_color(panel, example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY), 0.28),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR),
    };

    const double x = 2.0 * (0.5 * (fmax(-1.0, fmin(+1.0, amplitude)) + 1.0));
    const uint32_t segment = x >= 2.0 ? 3u : (uint32_t)floor(x * 2.0);
    const double local = x >= 2.0 ? 1.0 : x * 2.0 - (double)segment;
    return _mix_color(stops[segment], stops[segment + 1u], local);
}



/**
 * Map an absolute normalized amplitude to a smooth radial lobe coordinate.
 *
 * The offset smooth magnitude is zero at a harmonic node and has a finite zero slope there. This
 * avoids the cusp produced by pow(abs(x), p) when p is below one.
 *
 * @param settings harmonic settings
 * @param amplitude normalized signed amplitude in [-1, +1]
 * @return smooth lobe coordinate in [0, 1]
 */
static double _smooth_lobe(const HarmonicSettings* settings, double amplitude)
{
    if (settings == NULL)
        return 0.0;
    const double epsilon = (double)settings->node_smoothing;
    const double magnitude = sqrt(amplitude * amplitude + epsilon * epsilon) - epsilon;
    const double maximum = sqrt(1.0 + epsilon * epsilon) - epsilon;
    return pow(fmax(0.0, magnitude / maximum), (double)settings->lobe_power);
}



/**
 * Compute area-weighted vertex normals for the deformed indexed mesh.
 *
 * @param state initialized showcase state
 * @return whether every triangle index and output normal is valid
 */
static bool _compute_area_weighted_normals(SphericalHarmonicsState* state)
{
    if (state == NULL || state->geometry == NULL || state->geometry->positions == NULL ||
        state->geometry->normals == NULL || state->geometry->indices == NULL ||
        state->geometry->index_count % 3u != 0)
    {
        return false;
    }

    DvzGeometry* geometry = state->geometry;
    dvz_memset(
        geometry->normals, (size_t)geometry->vertex_count * sizeof(dvec3), 0,
        (size_t)geometry->vertex_count * sizeof(dvec3));

    for (uint32_t index = 0; index < geometry->index_count; index += 3u)
    {
        const DvzIndex i0 = geometry->indices[index + 0u];
        const DvzIndex i1 = geometry->indices[index + 1u];
        const DvzIndex i2 = geometry->indices[index + 2u];
        if (i0 >= geometry->vertex_count || i1 >= geometry->vertex_count ||
            i2 >= geometry->vertex_count)
        {
            return false;
        }

        dvec3 edge_a = {
            geometry->positions[i1][0] - geometry->positions[i0][0],
            geometry->positions[i1][1] - geometry->positions[i0][1],
            geometry->positions[i1][2] - geometry->positions[i0][2],
        };
        dvec3 edge_b = {
            geometry->positions[i2][0] - geometry->positions[i0][0],
            geometry->positions[i2][1] - geometry->positions[i0][1],
            geometry->positions[i2][2] - geometry->positions[i0][2],
        };
        const dvec3 normal = {
            edge_a[1] * edge_b[2] - edge_a[2] * edge_b[1],
            edge_a[2] * edge_b[0] - edge_a[0] * edge_b[2],
            edge_a[0] * edge_b[1] - edge_a[1] * edge_b[0],
        };
        for (uint32_t axis = 0; axis < 3u; axis++)
        {
            geometry->normals[i0][axis] += normal[axis];
            geometry->normals[i1][axis] += normal[axis];
            geometry->normals[i2][axis] += normal[axis];
        }
    }

    for (uint32_t vertex = 0; vertex < geometry->vertex_count; vertex++)
    {
        if (!_normalize(geometry->normals[vertex]))
            return false;
        const double radial_dot = geometry->normals[vertex][0] * state->directions[vertex][0] +
                                  geometry->normals[vertex][1] * state->directions[vertex][1] +
                                  geometry->normals[vertex][2] * state->directions[vertex][2];
        if (radial_dot < 0.0)
        {
            for (uint32_t axis = 0; axis < 3u; axis++)
                geometry->normals[vertex][axis] = -geometry->normals[vertex][axis];
        }
    }
    return true;
}



/**
 * Clamp live settings to the supported mathematical and visual ranges.
 *
 * @param settings settings to clamp
 */
static void _settings_clamp(HarmonicSettings* settings)
{
    if (settings == NULL)
        return;
    settings->phase = fmaxf(0.0f, fminf(1.0f, settings->phase));
    settings->speed = fmaxf(0.1f, fminf(3.0f, settings->speed));
    settings->base_radius = fmaxf(0.20f, fminf(0.90f, settings->base_radius));
    settings->lobe_scale = fmaxf(0.20f, fminf(1.60f, settings->lobe_scale));
    settings->lobe_power = fmaxf(0.65f, fminf(1.50f, settings->lobe_power));
    settings->node_smoothing = fmaxf(0.01f, fminf(0.35f, settings->node_smoothing));
    settings->breathing_amplitude = fmaxf(0.0f, fminf(0.12f, settings->breathing_amplitude));
    for (uint32_t i = 0; i < HARMONIC_TERM_COUNT; i++)
    {
        HarmonicTerm* term = &settings->terms[i];
        term->degree = term->degree < 0 ? 0 : (term->degree > 12 ? 12 : term->degree);
        term->order = term->order < -term->degree ? -term->degree : term->order;
        term->order = term->order > +term->degree ? +term->degree : term->order;
        term->base_weight = fmaxf(-1.50f, fminf(+1.50f, term->base_weight));
        term->morph_amplitude = fmaxf(0.0f, fminf(0.80f, term->morph_amplitude));
        term->morph_frequency = term->morph_frequency < 0
                                    ? 0
                                    : (term->morph_frequency > 6 ? 6 : term->morph_frequency);
        term->morph_phase = fmaxf(0.0f, fminf(1.0f, term->morph_phase));
    }
}



/**
 * Replace the current settings with one curated harmonic preset.
 *
 * @param state showcase state
 * @param preset preset index
 */
static void _load_preset(SphericalHarmonicsState* state, int preset)
{
    if (state == NULL)
        return;

    state->settings = HARMONIC_DEFAULTS;
    state->preset_index = preset;
    if (preset == 1)
    {
        state->settings.terms[0] = (HarmonicTerm){7, +3, +1.00f, 0.24f, 1, 0.00f};
        state->settings.terms[1] = (HarmonicTerm){5, -2, +0.48f, 0.18f, 2, 0.16f};
        state->settings.terms[2] = (HarmonicTerm){3, +3, -0.32f, 0.16f, 3, 0.37f};
        state->settings.terms[3] = (HarmonicTerm){2, 0, +0.20f, 0.12f, 1, 0.62f};
    }
    else if (preset == 2)
    {
        state->settings.terms[0] = (HarmonicTerm){8, 0, +1.00f, 0.20f, 1, 0.00f};
        state->settings.terms[1] = (HarmonicTerm){6, +1, +0.44f, 0.22f, 2, 0.25f};
        state->settings.terms[2] = (HarmonicTerm){4, -1, -0.30f, 0.18f, 3, 0.50f};
        state->settings.terms[3] = (HarmonicTerm){2, 0, +0.18f, 0.10f, 1, 0.75f};
    }
    state->time_initialized = false;
    state->basis_dirty = true;
    state->geometry_dirty = true;
}



/**
 * Recompute the cached basis after live degree or order edits.
 *
 * @param state initialized showcase state
 * @return whether the cache was updated
 */
static bool _recompute_basis(SphericalHarmonicsState* state)
{
    if (state == NULL || state->geometry == NULL || state->directions == NULL ||
        state->basis == NULL)
    {
        return false;
    }
    for (uint32_t vertex = 0; vertex < state->geometry->vertex_count; vertex++)
    {
        _harmonic_basis(
            state->directions[vertex], &state->settings,
            &state->basis[vertex * HARMONIC_TERM_COUNT]);
    }
    for (uint32_t i = 0; i < HARMONIC_TERM_COUNT; i++)
    {
        state->basis_degree[i] = state->settings.terms[i].degree;
        state->basis_order[i] = state->settings.terms[i].order;
    }
    state->basis_dirty = false;
    return true;
}



/**
 * Draw the live spherical-harmonic controls.
 *
 * @param gui GUI
 * @param user showcase state
 * @return whether any setting changed
 */
static bool _harmonic_settings_gui(DvzGui* gui, void* user)
{
    SphericalHarmonicsState* state = (SphericalHarmonicsState*)user;
    if (gui == NULL || state == NULL)
        return false;

    static const char* const presets[] = {"Showcase blend", "Sectoral bloom", "Zonal pulse"};
    bool changed = false;
    if (dvz_gui_combo(gui, "Preset", &state->preset_index, presets, DVZ_ARRAY_COUNT(presets)))
    {
        _load_preset(state, state->preset_index);
        changed = true;
    }

    HarmonicSettings* settings = &state->settings;
    dvz_gui_separator_text(gui, "Animation");
    changed |= dvz_gui_checkbox(gui, "Animate", &settings->animate);
    const bool phase_changed =
        dvz_gui_slider_float_format(gui, "Phase", &settings->phase, 0.0f, 1.0f, "%.3f");
    if (phase_changed)
        settings->animate = false;
    changed |= phase_changed;
    changed |= dvz_gui_slider_float_format(gui, "Speed", &settings->speed, 0.1f, 3.0f, "%.2fx");
    changed |= dvz_gui_slider_float_format(
        gui, "Breathing", &settings->breathing_amplitude, 0.0f, 0.12f, "%.3f");

    dvz_gui_separator_text(gui, "Surface");
    changed |= dvz_gui_slider_float_format(
        gui, "Base radius", &settings->base_radius, 0.20f, 0.90f, "%.2f");
    changed |= dvz_gui_slider_float_format(
        gui, "Lobe scale", &settings->lobe_scale, 0.20f, 1.60f, "%.2f");
    changed |= dvz_gui_slider_float_format(
        gui, "Lobe power", &settings->lobe_power, 0.65f, 1.50f, "%.2f");
    changed |= dvz_gui_slider_float_format(
        gui, "Node smoothing", &settings->node_smoothing, 0.01f, 0.35f, "%.3f");

    for (uint32_t i = 0; i < HARMONIC_TERM_COUNT; i++)
    {
        HarmonicTerm* term = &settings->terms[i];
        char label[64] = {0};
        dvz_snprintf(label, sizeof(label), "Term %u", i + 1u);
        dvz_gui_separator_text(gui, label);

        dvz_snprintf(label, sizeof(label), "Degree l##degree_%u", i);
        changed |= dvz_gui_slider_int(gui, label, &term->degree, 0, 12);
        term->order = term->order < -term->degree ? -term->degree : term->order;
        term->order = term->order > +term->degree ? +term->degree : term->order;
        dvz_snprintf(label, sizeof(label), "Order m##order_%u", i);
        changed |= dvz_gui_slider_int(gui, label, &term->order, -term->degree, +term->degree);
        dvz_snprintf(label, sizeof(label), "Coefficient##weight_%u", i);
        changed |=
            dvz_gui_slider_float_format(gui, label, &term->base_weight, -1.50f, +1.50f, "%+.2f");
        dvz_snprintf(label, sizeof(label), "Morph amplitude##amplitude_%u", i);
        changed |=
            dvz_gui_slider_float_format(gui, label, &term->morph_amplitude, 0.0f, 0.80f, "%.2f");
        dvz_snprintf(label, sizeof(label), "Frequency##frequency_%u", i);
        changed |= dvz_gui_slider_int(gui, label, &term->morph_frequency, 0, 6);
        dvz_snprintf(label, sizeof(label), "Phase offset##phase_%u", i);
        changed |=
            dvz_gui_slider_float_format(gui, label, &term->morph_phase, 0.0f, 1.0f, "%.2f cycle");
    }
    return changed;
}



/**
 * Apply live settings on the next scenario frame.
 *
 * @param user showcase state
 */
static void _harmonic_settings_apply(void* user)
{
    SphericalHarmonicsState* state = (SphericalHarmonicsState*)user;
    if (state == NULL)
        return;
    _settings_clamp(&state->settings);
    for (uint32_t i = 0; i < HARMONIC_TERM_COUNT; i++)
    {
        if (state->basis_degree[i] != state->settings.terms[i].degree ||
            state->basis_order[i] != state->settings.terms[i].order)
        {
            state->basis_dirty = true;
            break;
        }
    }
    state->geometry_dirty = true;
}



/**
 * Restore the showcase harmonic defaults.
 *
 * @param user showcase state
 */
static void _harmonic_settings_reset(void* user)
{
    SphericalHarmonicsState* state = (SphericalHarmonicsState*)user;
    if (state == NULL)
        return;
    state->settings = state->defaults;
    state->preset_index = 0;
    state->time_initialized = false;
    state->basis_dirty = true;
    state->geometry_dirty = true;
}



/**
 * Print pasteable C defaults for the live harmonic settings.
 *
 * @param fp output stream
 * @param user showcase state
 */
static void _harmonic_settings_print(FILE* fp, void* user)
{
    const SphericalHarmonicsState* state = (const SphericalHarmonicsState*)user;
    if (state == NULL)
        return;
    if (fp == NULL)
        fp = stdout;
    const HarmonicSettings* settings = &state->settings;
    fprintf(fp, "static const HarmonicSettings HARMONIC_DEFAULTS = {\n");
    fprintf(fp, "    .animate = %s,\n", settings->animate ? "true" : "false");
    fprintf(fp, "    .phase = %.3ff,\n", (double)settings->phase);
    fprintf(fp, "    .speed = %.3ff,\n", (double)settings->speed);
    fprintf(fp, "    .base_radius = %.3ff,\n", (double)settings->base_radius);
    fprintf(fp, "    .lobe_scale = %.3ff,\n", (double)settings->lobe_scale);
    fprintf(fp, "    .lobe_power = %.3ff,\n", (double)settings->lobe_power);
    fprintf(fp, "    .node_smoothing = %.3ff,\n", (double)settings->node_smoothing);
    fprintf(
        fp, "    .breathing_amplitude = %.3ff,\n    .terms = {\n",
        (double)settings->breathing_amplitude);
    for (uint32_t i = 0; i < HARMONIC_TERM_COUNT; i++)
    {
        const HarmonicTerm* term = &settings->terms[i];
        fprintf(
            fp, "        {%d, %+d, %+.3ff, %.3ff, %d, %.3ff},\n", term->degree, term->order,
            (double)term->base_weight, (double)term->morph_amplitude, term->morph_frequency,
            (double)term->morph_phase);
    }
    fprintf(fp, "    },\n};\n");
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
    _harmonic_weights(&state->settings, phase, weights);

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
    if (!isfinite(max_amplitude))
        return false;
    if (!(max_amplitude > 1e-12))
        max_amplitude = 1.0;

    const double breathing = 1.0 - (double)state->settings.breathing_amplitude +
                             (double)state->settings.breathing_amplitude * cos(2.0 * PI * phase);
    for (uint32_t vertex = 0; vertex < state->geometry->vertex_count; vertex++)
    {
        const double normalized = state->amplitudes[vertex] / max_amplitude;
        const double radius =
            (double)state->settings.base_radius + (double)state->settings.lobe_scale * breathing *
                                                      _smooth_lobe(&state->settings, normalized);
        for (uint32_t axis = 0; axis < 3u; axis++)
        {
            state->geometry->positions[vertex][axis] = radius * state->directions[vertex][axis];
        }
        state->geometry->colors[vertex] = _harmonic_color(normalized);
    }
    if (!_compute_area_weighted_normals(state))
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
    if (!_recompute_basis(state))
        goto cleanup;
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
static double _animation_phase(const DvzScenarioContext* ctx, SphericalHarmonicsState* state)
{
    if (ctx == NULL || state == NULL)
        return 0.0;
    if (ctx->preview_mode)
    {
        state->settings.phase =
            (float)dvz_scenario_preview_phase(ctx, DVZ_SCENARIO_PREVIEW_PHASE_SEAMLESS_LOOP);
        return (double)state->settings.phase;
    }

    const double time = fmax(0.0, ctx->time);
    if (!state->time_initialized)
    {
        state->last_time = time;
        state->time_initialized = true;
    }
    const double delta = fmax(0.0, time - state->last_time);
    state->last_time = time;
    if (state->settings.animate)
    {
        state->settings.phase = (float)fmod(
            (double)state->settings.phase +
                delta * (double)state->settings.speed / HARMONIC_MORPH_PERIOD,
            1.0);
    }
    return (double)state->settings.phase;
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
    state->tuner = example_tuner("Spherical harmonics");
    state->settings = HARMONIC_DEFAULTS;
    state->defaults = HARMONIC_DEFAULTS;
    state->basis_dirty = true;
    state->geometry_dirty = true;

    EXAMPLE_CHECK(_create_geometry(state), "spherical-harmonics geometry generation failed");

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");
    example_tuner_figure(&state->tuner, ctx->figure);

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    const float light_direction[3] = {-0.48f, +0.62f, +0.72f};
    EXAMPLE_CHECK(example_panel_directional_light(ctx->scene, panel, light_direction), "failed to configure spherical-harmonics light");
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

    state->material = example_default_standard_material_desc();
    state->material.standard.roughness = 0.58f;
    state->material.standard.specular = 0.16f;
    state->material.standard.rim_strength = 0.12f;
    const int material_result = dvz_visual_set_material(state->visual, &state->material);
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
    state->arcball = dvz_controller_arcball(controller);
    EXAMPLE_CHECK(state->arcball != NULL, "dvz_controller_arcball() failed");
    const int bind_result = dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ);
    EXAMPLE_CHECK(bind_result == DVZ_OK, "dvz_scenario_bind_controller() failed");
    vec3 initial_angles = {-0.233247f, +0.292663f, -0.038705f};
    vec2 initial_pan = {0.0f, 0.0f};
    dvz_arcball_initial(state->arcball, initial_angles);
    dvz_arcball_zoom(state->arcball, 0.606531f);
    dvz_arcball_pan(state->arcball, initial_pan);

    (void)example_tuner_add_component(
        &state->tuner, "Harmonic field", state, NULL, _harmonic_settings_gui,
        _harmonic_settings_apply, _harmonic_settings_reset, _harmonic_settings_print);
    example_tuner_arcball(
        &state->tuner, "Arcball", state->arcball, initial_angles, 0.606531f, initial_pan);
    example_tuner_material(&state->tuner, "Surface material", state->visual, &state->material);

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
    const double phase = _animation_phase(ctx, state);
    if (state->basis_dirty && !_recompute_basis(state))
        return;
    if (!ctx->preview_mode && !state->settings.animate && !state->geometry_dirty)
        return;
    if (!_update_geometry(state, phase))
        return;
    if (_upload_geometry(state))
        state->geometry_dirty = false;
}



/**
 * Attach the native live tuner after the view exists.
 *
 * @param ctx scenario context
 * @param app native app
 * @param view native view
 * @param user scenario state
 * @return whether the tuner was attached or intentionally skipped
 */
static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    SphericalHarmonicsState* state = (SphericalHarmonicsState*)user;
    if (ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
    {
        return true;
    }
    return example_tuner_attach(&state->tuner, view);
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
    example_tuner_detach(&state->tuner);
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
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == DVZ_OK ? 0 : 1;
}
#endif
