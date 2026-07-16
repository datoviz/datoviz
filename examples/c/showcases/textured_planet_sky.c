/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "textured_planet_sky.h"

#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_compat.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define SKY_MAGIC       "DVZSKY1"
#define SKY_MAGIC_SIZE  8u
#define SKY_VERSION     1u
#define SKY_MAX_OBJECTS 1000000u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Read exactly one byte range from a file.
 *
 * @param file input file
 * @param data destination
 * @param size byte count
 * @return whether the complete range was read
 */
static bool _read_exact(FILE* file, void* data, size_t size)
{
    return file != NULL && data != NULL && (size == 0 || fread(data, 1, size, file) == size);
}



/**
 * Allocate and read one prepared sky layer.
 *
 * @param file input file
 * @param count layer item count
 * @param layer output layer
 * @return whether the layer was read
 */
static bool _read_layer(FILE* file, uint32_t count, TexturedPlanetSkyLayer* layer)
{
    if (file == NULL || layer == NULL || count == 0 || count > SKY_MAX_OBJECTS)
        return false;
    layer->count = count;
    layer->positions = (vec3*)dvz_calloc(count, sizeof(vec3));
    layer->colors = (DvzColor*)dvz_calloc(count, sizeof(DvzColor));
    layer->sizes = (float*)dvz_calloc(count, sizeof(float));
    if (layer->positions == NULL || layer->colors == NULL || layer->sizes == NULL)
        return false;
    return _read_exact(file, layer->positions, (size_t)count * sizeof(vec3)) &&
           _read_exact(file, layer->colors, (size_t)count * sizeof(DvzColor)) &&
           _read_exact(file, layer->sizes, (size_t)count * sizeof(float));
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool textured_planet_sky_model_load(const char* path, TexturedPlanetSkyModel* model)
{
    if (path == NULL || model == NULL)
        return false;
    textured_planet_sky_model_destroy(model);

    FILE* file = fopen(path, "rb");
    if (file == NULL)
        return false;

    bool ok = false;
    char magic[SKY_MAGIC_SIZE] = {0};
    uint32_t version = 0;
    uint32_t star_count = 0;
    uint32_t galaxy_count = 0;
    uint32_t reserved = 0;
    if (!_read_exact(file, magic, sizeof(magic)) ||
        !_read_exact(file, &version, sizeof(version)) ||
        !_read_exact(file, &star_count, sizeof(star_count)) ||
        !_read_exact(file, &galaxy_count, sizeof(galaxy_count)) ||
        !_read_exact(file, &reserved, sizeof(reserved)) ||
        !_read_exact(file, model->snapshot_utc, sizeof(model->snapshot_utc)))
    {
        goto cleanup;
    }
    (void)reserved;
    if (memcmp(magic, SKY_MAGIC, strlen(SKY_MAGIC)) != 0 || version != SKY_VERSION)
        goto cleanup;
    model->snapshot_utc[sizeof(model->snapshot_utc) - 1] = '\0';
    if (!_read_layer(file, star_count, &model->stars) ||
        !_read_layer(file, galaxy_count, &model->galaxy))
    {
        goto cleanup;
    }
    if (fgetc(file) != EOF)
        goto cleanup;
    ok = true;

cleanup:
    fclose(file);
    if (!ok)
        textured_planet_sky_model_destroy(model);
    return ok;
}



void textured_planet_sky_model_destroy(TexturedPlanetSkyModel* model)
{
    if (model == NULL)
        return;
    dvz_free(model->stars.positions);
    dvz_free(model->stars.colors);
    dvz_free(model->stars.sizes);
    dvz_free(model->galaxy.positions);
    dvz_free(model->galaxy.colors);
    dvz_free(model->galaxy.sizes);
    dvz_memset(model, sizeof(*model), 0, sizeof(*model));
}
