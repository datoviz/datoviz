/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 *
 * The density-wave model is adapted from Glumpy's galaxy_simulation.py:
 * Copyright (c) 2014 Nicolas P. Rougier.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this list of conditions
 *   and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice, this list of
 *   conditions and the following disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 * - Neither the name of Nicolas P. Rougier nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Stellar colors reproduce the public-domain specrend black-body reference values used by Glumpy.
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "galaxy_model.h"

#include <math.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define GALAXY_RADIUS         13000.0f
#define GALAXY_CORE_RADIUS    4000.0f
#define GALAXY_NORMALIZATION  27000.0f
#define GALAXY_ANGULAR_OFFSET 0.0004f
#define GALAXY_ECCENTRICITY   0.90f

static const float PI = 3.14159265358979323846f;

static const float BLACK_BODY_RGB[19][3] = {
    {1.000f, 0.007f, 0.000f}, {1.000f, 0.126f, 0.000f}, {1.000f, 0.234f, 0.010f},
    {1.000f, 0.349f, 0.067f}, {1.000f, 0.454f, 0.151f}, {1.000f, 0.549f, 0.254f},
    {1.000f, 0.635f, 0.370f}, {1.000f, 0.710f, 0.493f}, {1.000f, 0.778f, 0.620f},
    {1.000f, 0.837f, 0.746f}, {1.000f, 0.890f, 0.869f}, {1.000f, 0.937f, 0.988f},
    {0.907f, 0.888f, 1.000f}, {0.827f, 0.839f, 1.000f}, {0.762f, 0.800f, 1.000f},
    {0.711f, 0.766f, 1.000f}, {0.668f, 0.738f, 1.000f}, {0.632f, 0.714f, 1.000f},
    {0.602f, 0.693f, 1.000f},
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GalaxyRandom
{
    uint32_t state;
    bool has_spare;
    float spare;
} GalaxyRandom;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a deterministic pseudorandom unsigned integer.
 *
 * @param random random generator state
 * @return pseudorandom integer
 */
static uint32_t _random_u32(GalaxyRandom* random)
{
    ANN(random);
    uint32_t x = random->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    random->state = x;
    return x;
}



/**
 * Return a deterministic uniform sample in [0, 1).
 *
 * @param random random generator state
 * @return uniform sample
 */
static float _random_uniform(GalaxyRandom* random)
{
    return (float)(_random_u32(random) >> 8) * (1.0f / 16777216.0f);
}



/**
 * Return a deterministic standard-normal sample.
 *
 * @param random random generator state
 * @return standard-normal sample
 */
static float _random_normal(GalaxyRandom* random)
{
    ANN(random);
    if (random->has_spare)
    {
        random->has_spare = false;
        return random->spare;
    }
    const float u = fmaxf(_random_uniform(random), 1e-7f);
    const float v = _random_uniform(random);
    const float radius = sqrtf(-2.0f * logf(u));
    const float angle = 2.0f * PI * v;
    random->spare = radius * sinf(angle);
    random->has_spare = true;
    return radius * cosf(angle);
}



/**
 * Return the density-wave ellipse eccentricity at one radius.
 *
 * @param radius galactic radius
 * @return ellipse minor-to-major ratio
 */
static float _eccentricity(float radius)
{
    if (radius < GALAXY_CORE_RADIUS)
    {
        return 1.0f + (radius / GALAXY_CORE_RADIUS) * (GALAXY_ECCENTRICITY - 1.0f);
    }
    if (radius <= GALAXY_RADIUS)
    {
        return GALAXY_ECCENTRICITY;
    }
    if (radius < 2.0f * GALAXY_RADIUS)
    {
        const float t = (radius - GALAXY_RADIUS) / GALAXY_RADIUS;
        return GALAXY_ECCENTRICITY + t * (1.0f - GALAXY_ECCENTRICITY);
    }
    return 1.0f;
}



/**
 * Interpolate the black-body color table used by the original Glumpy example.
 *
 * @param temperature temperature in kelvin
 * @param rgb output normalized RGB
 */
static void _black_body_rgb(float temperature, float rgb[3])
{
    ANN(rgb);
    const float x = fminf(fmaxf((temperature - 1000.0f) / 500.0f, 0.0f), 18.0f);
    const uint32_t i0 = (uint32_t)floorf(x);
    const uint32_t i1 = i0 < 18u ? i0 + 1u : i0;
    const float t = x - (float)i0;
    for (uint32_t channel = 0; channel < 3; channel++)
        rgb[channel] = (1.0f - t) * BLACK_BODY_RGB[i0][channel] + t * BLACK_BODY_RGB[i1][channel];
}



/**
 * Convert normalized color and particle brightness to the retained RGBA style.
 *
 * @param particle source particle
 * @return retained color
 */
static DvzColor _particle_color(const GalaxyParticle* particle)
{
    ANN(particle);
    float rgb[3] = {0};
    _black_body_rgb(particle->temperature, rgb);
    float brightness = 1.8f * particle->brightness;
    if (particle->type == GALAXY_PARTICLE_HII_GLOW)
    {
        rgb[0] = fminf(2.0f * rgb[0], 1.0f);
        brightness *= 2.0f;
    }
    else if (particle->type == GALAXY_PARTICLE_HII_CORE)
    {
        rgb[0] = rgb[1] = rgb[2] = 0.90f;
        brightness *= 1.6f;
    }
    return dvz_color_rgba(
        (uint8_t)(255.0f * rgb[0] + 0.5f), (uint8_t)(255.0f * rgb[1] + 0.5f),
        (uint8_t)(255.0f * rgb[2] + 0.5f), (uint8_t)(255.0f * fminf(brightness, 1.0f) + 0.5f));
}



/**
 * Initialize one particle's shared density-wave parameters.
 *
 * @param particle particle to initialize
 * @param radius ellipse major radius
 * @param orientation ellipse orientation in radians
 * @param phase_degrees orbital phase in degrees
 */
static void
_particle_orbit(GalaxyParticle* particle, float radius, float orientation, float phase_degrees)
{
    ANN(particle);
    particle->major_radius = radius;
    particle->minor_radius = radius * _eccentricity(radius);
    particle->orientation = orientation;
    particle->phase_degrees = phase_degrees;
    particle->angular_velocity = 0.000005f;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize a deterministic density-wave galaxy.
 *
 * @param model output model
 * @param seed deterministic nonzero seed
 * @return whether initialization succeeded
 */
bool galaxy_model_init(GalaxyModel* model, uint32_t seed)
{
    ANN(model);
    dvz_memset(model, sizeof(*model), 0, sizeof(*model));
    model->particle_count = GALAXY_PARTICLE_COUNT;
    model->particles = dvz_calloc(model->particle_count, sizeof(*model->particles));
    model->positions = dvz_calloc(model->particle_count, sizeof(*model->positions));
    model->colors = dvz_calloc(model->particle_count, sizeof(*model->colors));
    model->sizes = dvz_calloc(model->particle_count, sizeof(*model->sizes));
    if (model->particles == NULL || model->positions == NULL || model->colors == NULL ||
        model->sizes == NULL)
    {
        galaxy_model_destroy(model);
        return false;
    }

    GalaxyRandom random = {.state = seed != 0 ? seed : 0x91E10DA5u};
    for (uint32_t i = 0; i < GALAXY_STAR_COUNT; i++)
    {
        GalaxyParticle* particle = &model->particles[i];
        const float radius = 0.5f * GALAXY_RADIUS * _random_normal(&random);
        _particle_orbit(
            particle, radius, 90.0f - radius * GALAXY_ANGULAR_OFFSET,
            360.0f * _random_uniform(&random));
        particle->type = GALAXY_PARTICLE_STAR;
        particle->temperature = 3000.0f + 6000.0f * _random_uniform(&random);
        particle->brightness = 0.05f + 0.20f * _random_uniform(&random);
        particle->base_size_px = 3.0f;
    }

    const uint32_t dust_first = GALAXY_STAR_COUNT;
    for (uint32_t i = 0; i < GALAXY_DUST_COUNT; i++)
    {
        GalaxyParticle* particle = &model->particles[dust_first + i];
        const float x = 2.0f * GALAXY_RADIUS * _random_uniform(&random);
        const float y = GALAXY_RADIUS * (2.0f * _random_uniform(&random) - 1.0f);
        const float radius = sqrtf(x * x + y * y);
        _particle_orbit(
            particle, radius, radius * GALAXY_ANGULAR_OFFSET, 360.0f * _random_uniform(&random));
        particle->type = GALAXY_PARTICLE_DUST;
        particle->temperature = 6000.0f + 0.25f * radius;
        particle->brightness = 0.01f + 0.01f * _random_uniform(&random);
        particle->base_size_px = 64.0f;
    }

    const uint32_t hii_first = dust_first + GALAXY_DUST_COUNT;
    for (uint32_t i = 0; i < GALAXY_HII_COUNT; i++)
    {
        const float x = GALAXY_RADIUS * (2.0f * _random_uniform(&random) - 1.0f);
        const float y = GALAXY_RADIUS * (2.0f * _random_uniform(&random) - 1.0f);
        const float radius = sqrtf(x * x + y * y);
        const float phase = 360.0f * _random_uniform(&random);
        const float temperature = 3000.0f + 6000.0f * _random_uniform(&random);
        const float brightness = 0.005f + 0.005f * _random_uniform(&random);
        GalaxyParticle* glow = &model->particles[hii_first + i];
        GalaxyParticle* core = &model->particles[hii_first + GALAXY_HII_COUNT + i];
        _particle_orbit(glow, radius, radius * GALAXY_ANGULAR_OFFSET, phase);
        _particle_orbit(core, radius + 1000.0f, radius * GALAXY_ANGULAR_OFFSET, phase);
        glow->type = GALAXY_PARTICLE_HII_GLOW;
        core->type = GALAXY_PARTICLE_HII_CORE;
        glow->temperature = core->temperature = temperature;
        glow->brightness = core->brightness = brightness;
    }

    for (uint32_t i = 0; i < model->particle_count; i++)
        model->colors[i] = _particle_color(&model->particles[i]);
    galaxy_model_update(model, 100000.0);
    return true;
}



/**
 * Update all particle positions at an absolute elapsed simulation time.
 *
 * @param model galaxy model
 * @param elapsed_years elapsed simulation time in years
 */
void galaxy_model_update(GalaxyModel* model, double elapsed_years)
{
    ANN(model);
    ANN(model->particles);
    ANN(model->positions);
    ANN(model->sizes);

    for (uint32_t i = 0; i < model->particle_count; i++)
    {
        const GalaxyParticle* particle = &model->particles[i];
        const float theta =
            (particle->phase_degrees + particle->angular_velocity * (float)elapsed_years) * PI /
            180.0f;
        const float beta = -particle->orientation;
        const float cos_theta = cosf(theta);
        const float sin_theta = sinf(theta);
        const float cos_beta = cosf(beta);
        const float sin_beta = sinf(beta);
        const float x = particle->major_radius * cos_theta * cos_beta -
                        particle->minor_radius * sin_theta * sin_beta;
        const float y = particle->major_radius * cos_theta * sin_beta +
                        particle->minor_radius * sin_theta * cos_beta;
        model->positions[i][0] = x / GALAXY_NORMALIZATION;
        model->positions[i][1] = y / GALAXY_NORMALIZATION;
        model->positions[i][2] = 0.0f;
        model->sizes[i] = particle->base_size_px;
    }

    const uint32_t hii_first = GALAXY_STAR_COUNT + GALAXY_DUST_COUNT;
    for (uint32_t i = 0; i < GALAXY_HII_COUNT; i++)
    {
        const uint32_t glow_index = hii_first + i;
        const uint32_t core_index = hii_first + GALAXY_HII_COUNT + i;
        const float dx = (model->positions[glow_index][0] - model->positions[core_index][0]) *
                         GALAXY_NORMALIZATION;
        const float dy = (model->positions[glow_index][1] - model->positions[core_index][1]) *
                         GALAXY_NORMALIZATION;
        const float distance = sqrtf(dx * dx + dy * dy);
        const float scale = fmaxf(1.0f, (1000.0f - distance) / 10.0f - 50.0f);
        const float glow_size = 2.0f * scale;
        const float core_size = scale / 6.0f;
        model->sizes[glow_index] = glow_size > 2.0f ? fminf(glow_size, 64.0f) : 0.0f;
        model->sizes[core_index] = core_size > 2.0f ? fminf(core_size, 64.0f) : 0.0f;
    }
}



/**
 * Destroy all model-owned particle arrays.
 *
 * @param model galaxy model
 */
void galaxy_model_destroy(GalaxyModel* model)
{
    if (model == NULL)
        return;
    dvz_free(model->sizes);
    dvz_free(model->colors);
    dvz_free(model->positions);
    dvz_free(model->particles);
    dvz_memset(model, sizeof(*model), 0, sizeof(*model));
}
