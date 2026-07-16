/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "textured_planet_orbits.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define ORBIT_MAGIC           "DVZORB1"
#define ORBIT_MAGIC_SIZE      8u
#define ORBIT_VERSION         1u
#define ORBIT_MAX_OBJECTS     100000u
#define ORBIT_MAX_FRAMES      10000u
#define ORBIT_MAX_EVENT_COUNT 16u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Read one exact byte range.
 *
 * @param fp input stream
 * @param data output buffer
 * @param size byte count
 * @return whether every byte was read
 */
static bool _read_exact(FILE* fp, void* data, size_t size)
{
    return fp != NULL && data != NULL && size > 0 && fread(data, 1, size, fp) == size;
}



/**
 * Interpolate one object's position without wrapping the requested time.
 *
 * @param model orbit model
 * @param object_index object index
 * @param time_s clamped ephemeris time
 * @param position output position
 */
static void _interpolate_position(
    const TexturedPlanetOrbitModel* model, uint32_t object_index, double time_s, float position[3])
{
    const double frame = time_s / model->step_seconds;
    uint32_t frame0 = (uint32_t)floor(frame);
    if (frame0 >= model->frame_count - 1)
        frame0 = model->frame_count - 1;
    const uint32_t frame1 = frame0 + 1 < model->frame_count ? frame0 + 1 : frame0;
    const float alpha = frame1 > frame0 ? (float)(frame - (double)frame0) : 0.0f;
    const float* p0 = model->ephemeris[(size_t)frame0 * model->count + object_index];
    const float* p1 = model->ephemeris[(size_t)frame1 * model->count + object_index];
    for (uint32_t component = 0; component < 3; component++)
        position[component] = p0[component] + alpha * (p1[component] - p0[component]);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Load a prepared real orbital-debris ephemeris.
 *
 * @param path prepared DVZORB1 binary path
 * @param model output orbit model
 * @return whether loading succeeded
 */
bool textured_planet_orbit_model_load(const char* path, TexturedPlanetOrbitModel* model)
{
    if (path == NULL || model == NULL)
        return false;
    memset(model, 0, sizeof(*model));

    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        return false;

    bool ok = false;
    char magic[ORBIT_MAGIC_SIZE] = {0};
    uint32_t version = 0;
    uint32_t reserved = 0;
    float earth_radius_km = 0.0f;
    if (!_read_exact(fp, magic, sizeof(magic)) || !_read_exact(fp, &version, sizeof(version)) ||
        !_read_exact(fp, &model->count, sizeof(model->count)) ||
        !_read_exact(fp, &model->frame_count, sizeof(model->frame_count)) ||
        !_read_exact(fp, &model->event_count, sizeof(model->event_count)) ||
        !_read_exact(fp, &reserved, sizeof(reserved)) ||
        !_read_exact(fp, &model->start_unix_s, sizeof(model->start_unix_s)) ||
        !_read_exact(fp, &model->step_seconds, sizeof(model->step_seconds)) ||
        !_read_exact(fp, &earth_radius_km, sizeof(earth_radius_km)) ||
        !_read_exact(fp, &model->max_radius, sizeof(model->max_radius)) ||
        !_read_exact(fp, model->snapshot_utc, sizeof(model->snapshot_utc)))
    {
        goto cleanup;
    }
    model->snapshot_utc[sizeof(model->snapshot_utc) - 1] = '\0';
    if (memcmp(magic, ORBIT_MAGIC, strlen(ORBIT_MAGIC)) != 0 || version != ORBIT_VERSION ||
        model->count == 0 || model->count > ORBIT_MAX_OBJECTS || model->frame_count < 2 ||
        model->frame_count > ORBIT_MAX_FRAMES || model->event_count == 0 ||
        model->event_count > ORBIT_MAX_EVENT_COUNT || model->step_seconds <= 0.0 ||
        !isfinite(model->step_seconds) || earth_radius_km <= 0.0f || !isfinite(model->max_radius))
    {
        goto cleanup;
    }

    const size_t object_count = model->count;
    if (model->frame_count > SIZE_MAX / object_count)
        goto cleanup;
    const size_t position_count = (size_t)model->frame_count * object_count;
    if (position_count > SIZE_MAX / sizeof(*model->ephemeris))
        goto cleanup;

    model->event_ids = (uint8_t*)dvz_calloc(object_count, sizeof(*model->event_ids));
    model->catalog_ids = (uint32_t*)dvz_calloc(object_count, sizeof(*model->catalog_ids));
    model->ephemeris = (float(*)[3])dvz_calloc(position_count, sizeof(*model->ephemeris));
    if (model->event_ids == NULL || model->catalog_ids == NULL || model->ephemeris == NULL)
        goto cleanup;
    if (!_read_exact(fp, model->event_ids, object_count * sizeof(*model->event_ids)) ||
        !_read_exact(fp, model->catalog_ids, object_count * sizeof(*model->catalog_ids)) ||
        !_read_exact(fp, model->ephemeris, position_count * sizeof(*model->ephemeris)))
    {
        goto cleanup;
    }

    for (uint32_t i = 0; i < model->count; i++)
    {
        if (model->event_ids[i] >= model->event_count)
            goto cleanup;
    }
    model->duration_seconds = model->step_seconds * (double)(model->frame_count - 1);
    ok = true;

cleanup:
    fclose(fp);
    if (!ok)
        textured_planet_orbit_model_destroy(model);
    return ok;
}



/**
 * Destroy an orbit model.
 *
 * @param model orbit model
 */
void textured_planet_orbit_model_destroy(TexturedPlanetOrbitModel* model)
{
    if (model == NULL)
        return;
    dvz_free(model->event_ids);
    dvz_free(model->catalog_ids);
    dvz_free(model->ephemeris);
    memset(model, 0, sizeof(*model));
}



/**
 * Interpolate every catalog-object position at one ephemeris time.
 *
 * @param model orbit model
 * @param time_s seconds after the prepared ephemeris start
 * @param positions output array containing at least model->count positions
 */
void textured_planet_orbit_model_positions(
    const TexturedPlanetOrbitModel* model, double time_s, float (*positions)[3])
{
    if (model == NULL || model->ephemeris == NULL || model->count == 0 ||
        model->duration_seconds <= 0.0 || positions == NULL)
        return;

    double wrapped_time = fmod(time_s, model->duration_seconds);
    if (wrapped_time < 0.0)
        wrapped_time += model->duration_seconds;
    for (uint32_t i = 0; i < model->count; i++)
        _interpolate_position(model, i, wrapped_time, positions[i]);
}



/**
 * Sample one catalog object's prepared trajectory.
 *
 * @param model orbit model
 * @param object_index catalog-object index
 * @param sample_count number of output positions
 * @param positions output array containing at least sample_count positions
 */
void textured_planet_orbit_model_trace(
    const TexturedPlanetOrbitModel* model, uint32_t object_index, uint32_t sample_count,
    float (*positions)[3])
{
    if (model == NULL || model->ephemeris == NULL || object_index >= model->count ||
        sample_count < 2 || positions == NULL)
        return;

    const double denominator = (double)(sample_count - 1);
    for (uint32_t i = 0; i < sample_count; i++)
    {
        const double time_s = model->duration_seconds * (double)i / denominator;
        _interpolate_position(model, object_index, time_s, positions[i]);
    }
}
