/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#ifndef TEXTURED_PLANET_ORBITS_H
#define TEXTURED_PLANET_ORBITS_H

#include <stdbool.h>
#include <stdint.h>



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct TexturedPlanetOrbitModel
{
    uint8_t* event_ids;
    uint32_t* catalog_ids;
    float (*ephemeris)[3];
    float (*closed_traces)[3];
    uint32_t count;
    uint32_t frame_count;
    uint32_t event_count;
    uint32_t trace_sample_count;
    double start_unix_s;
    double step_seconds;
    double duration_seconds;
    float max_radius;
    char snapshot_utc[32];
} TexturedPlanetOrbitModel;



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
bool textured_planet_orbit_model_load(const char* path, TexturedPlanetOrbitModel* model);



/**
 * Destroy an orbit model.
 *
 * @param model orbit model
 */
void textured_planet_orbit_model_destroy(TexturedPlanetOrbitModel* model);



/**
 * Interpolate every catalog-object position at one ephemeris time.
 *
 * @param model orbit model
 * @param time_s seconds after the prepared ephemeris start
 * @param positions output array containing at least model->count positions
 */
void textured_planet_orbit_model_positions(
    const TexturedPlanetOrbitModel* model, double time_s, float (*positions)[3]);



/**
 * Sample one catalog object's closed full-period trajectory.
 *
 * @param model orbit model
 * @param object_index catalog-object index
 * @param sample_count number of output positions
 * @param positions output array containing at least sample_count positions
 */
void textured_planet_orbit_model_trace(
    const TexturedPlanetOrbitModel* model, uint32_t object_index, uint32_t sample_count,
    float (*positions)[3]);

#endif
