/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene colormaps                                                                              */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "colormap_internal.h"
#include "scale_internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_COLORMAP_DESC_KNOWN_FLAGS 0u


/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Validate public colormap descriptor ABI fields.
 *
 * @param desc the colormap descriptor
 * @return whether the descriptor is accepted
 */
static bool _colormap_desc_validate(const DvzColormapDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzColormapDesc, DVZ_COLORMAP_DESC_KNOWN_FLAGS))
    {
        log_error("invalid colormap descriptor ABI");
        return false;
    }
    return true;
}


/**
 * Return the default colormap descriptor.
 *
 * @return default colormap descriptor
 */
DvzColormapDesc dvz_colormap_desc(void)
{
    return (DvzColormapDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzColormapDesc),
        .kind = DVZ_COLORMAP_CONTINUOUS,
        .builtin = DVZ_BUILTIN_COLORMAP_NONE,
    };
}


/**
 * Sample an ordered colormap stop table.
 *
 * @param stops the stop table
 * @param count number of stops
 * @param t normalized scalar value
 * @param out_rgba the output RGBA color
 * @return true when a color was written
 */
static bool _colormap_sample_stops(
    const DvzColormapStop* stops, uint32_t count, double t, uint8_t out_rgba[4])
{
    ANN(stops);
    ANN(out_rgba);
    if (count < 2)
        return false;
    const DvzColormapStop* lo = &stops[0];
    const DvzColormapStop* hi = &stops[count - 1];
    for (uint32_t i = 1; i < count; i++)
    {
        if (t <= stops[i].position)
        {
            lo = &stops[i - 1];
            hi = &stops[i];
            break;
        }
    }
    double span = hi->position - lo->position;
    double u = span > 0.0 ? (t - lo->position) / span : 0.0;
    if (u < 0.0)
        u = 0.0;
    if (u > 1.0)
        u = 1.0;
    for (uint32_t c = 0; c < 4; c++)
    {
        double value = (1.0 - u) * lo->rgba[c] + u * hi->rgba[c];
        out_rgba[c] = (uint8_t)(value + 0.5);
    }
    return true;
}


/**
 * Sample a custom colormap lookup table.
 *
 * @param colors the LUT colors
 * @param count number of LUT colors
 * @param t normalized scalar value
 * @param out_rgba the output RGBA color
 * @return true when a color was written
 */
static bool _colormap_sample_lut(
    const DvzColor* colors, uint32_t count, double t, uint8_t out_rgba[4])
{
    ANN(colors);
    ANN(out_rgba);
    if (count < 2)
        return false;

    const double x = t * (double)(count - 1u);
    uint32_t i0 = (uint32_t)x;
    if (i0 >= count - 1u)
        i0 = count - 2u;
    const uint32_t i1 = i0 + 1u;
    double u = x - (double)i0;
    if (u < 0.0)
        u = 0.0;
    if (u > 1.0)
        u = 1.0;

    const uint8_t lo[4] = {colors[i0].r, colors[i0].g, colors[i0].b, colors[i0].a};
    const uint8_t hi[4] = {colors[i1].r, colors[i1].g, colors[i1].b, colors[i1].a};
    for (uint32_t c = 0; c < 4; c++)
    {
        double value = (1.0 - u) * lo[c] + u * hi[c];
        out_rgba[c] = (uint8_t)(value + 0.5);
    }
    return true;
}


/**
 * Release custom LUT storage owned by a colormap.
 *
 * @param colormap the colormap
 */
static void _colormap_release_lut(DvzColormap* colormap)
{
    if (colormap == NULL)
        return;
    if (colormap->lut != NULL)
        dvz_free(colormap->lut);
    colormap->lut = NULL;
    colormap->lut_count = 0;
}



/**
 * Return a compact built-in colormap stop table.
 *
 * @param builtin the built-in colormap
 * @param out_count output stop count
 * @return the static stop table, or NULL
 */
static const DvzColormapStop*
_colormap_builtin_stops(DvzBuiltinColormap builtin, uint32_t* out_count)
{
    ANN(out_count);
    static const DvzColormapStop viridis[] = {
        {.position = 0.00, .rgba = {68, 1, 84, 255}},
        {.position = 0.25, .rgba = {59, 82, 139, 255}},
        {.position = 0.50, .rgba = {33, 145, 140, 255}},
        {.position = 0.75, .rgba = {94, 201, 98, 255}},
        {.position = 1.00, .rgba = {253, 231, 37, 255}},
    };
    static const DvzColormapStop magma[] = {
        {.position = 0.00, .rgba = {0, 0, 4, 255}},
        {.position = 0.25, .rgba = {80, 18, 123, 255}},
        {.position = 0.50, .rgba = {182, 54, 121, 255}},
        {.position = 0.75, .rgba = {251, 136, 97, 255}},
        {.position = 1.00, .rgba = {252, 253, 191, 255}},
    };
    static const DvzColormapStop plasma[] = {
        {.position = 0.00, .rgba = {13, 8, 135, 255}},
        {.position = 0.25, .rgba = {126, 3, 168, 255}},
        {.position = 0.50, .rgba = {204, 71, 120, 255}},
        {.position = 0.75, .rgba = {248, 149, 64, 255}},
        {.position = 1.00, .rgba = {240, 249, 33, 255}},
    };
    static const DvzColormapStop inferno[] = {
        {.position = 0.00, .rgba = {0, 0, 4, 255}},
        {.position = 0.25, .rgba = {87, 16, 110, 255}},
        {.position = 0.50, .rgba = {188, 55, 84, 255}},
        {.position = 0.75, .rgba = {249, 142, 9, 255}},
        {.position = 1.00, .rgba = {252, 255, 164, 255}},
    };
    static const DvzColormapStop cividis[] = {
        {.position = 0.00, .rgba = {0, 32, 76, 255}},
        {.position = 0.25, .rgba = {59, 78, 109, 255}},
        {.position = 0.50, .rgba = {124, 123, 120, 255}},
        {.position = 0.75, .rgba = {188, 172, 103, 255}},
        {.position = 1.00, .rgba = {255, 233, 69, 255}},
    };
    static const DvzColormapStop turbo[] = {
        {.position = 0.00, .rgba = {48, 18, 59, 255}},
        {.position = 0.20, .rgba = {55, 91, 178, 255}},
        {.position = 0.40, .rgba = {49, 205, 207, 255}},
        {.position = 0.60, .rgba = {135, 255, 88, 255}},
        {.position = 0.80, .rgba = {255, 170, 36, 255}},
        {.position = 1.00, .rgba = {122, 4, 3, 255}},
    };
    static const DvzColormapStop gray[] = {
        {.position = 0.00, .rgba = {0, 0, 0, 255}},
        {.position = 1.00, .rgba = {255, 255, 255, 255}},
    };
    switch (builtin)
    {
    case DVZ_BUILTIN_COLORMAP_VIRIDIS:
        *out_count = DVZ_ARRAY_COUNT(viridis);
        return viridis;
    case DVZ_BUILTIN_COLORMAP_MAGMA:
        *out_count = DVZ_ARRAY_COUNT(magma);
        return magma;
    case DVZ_BUILTIN_COLORMAP_PLASMA:
        *out_count = DVZ_ARRAY_COUNT(plasma);
        return plasma;
    case DVZ_BUILTIN_COLORMAP_INFERNO:
        *out_count = DVZ_ARRAY_COUNT(inferno);
        return inferno;
    case DVZ_BUILTIN_COLORMAP_CIVIDIS:
        *out_count = DVZ_ARRAY_COUNT(cividis);
        return cividis;
    case DVZ_BUILTIN_COLORMAP_TURBO:
        *out_count = DVZ_ARRAY_COUNT(turbo);
        return turbo;
    case DVZ_BUILTIN_COLORMAP_GRAY:
        *out_count = DVZ_ARRAY_COUNT(gray);
        return gray;
    case DVZ_BUILTIN_COLORMAP_NONE:
    default:
        *out_count = 0;
        return NULL;
    }
}


/**
 * Mark visuals depending on one colormap as needing refreshed texture data.
 *
 * @param colormap the colormap
 */
static void _scene_mark_colormap_dirty(DvzColormap* colormap)
{
    if (colormap == NULL || colormap->scene == NULL)
        return;
    DvzScene* scene = colormap->scene;
    for (uint32_t i = 0; i < scene->scale_count; i++)
    {
        DvzScale* scale = &scene->scales[i];
        if (scale->scene == scene && scale->colormap == colormap)
            _scene_mark_scale_dirty(scale);
    }
}


/**
 * Resolve one RGBA color from a retained colormap.
 *
 * @param colormap the colormap, or NULL for grayscale fallback
 * @param t the normalized scalar value
 * @param out_rgba the output RGBA color
 * @return true when a color was written
 */
bool _scene_color_from_colormap(
    const DvzColormap* colormap, double t, uint8_t out_rgba[4])
{
    ANN(out_rgba);
    if (t < 0.0)
        t = 0.0;
    if (t > 1.0)
        t = 1.0;

    if (colormap != NULL && colormap->stop_count >= 2)
    {
        return _colormap_sample_stops(colormap->stops, colormap->stop_count, t, out_rgba);
    }

    if (colormap != NULL && colormap->lut != NULL && colormap->lut_count >= 2)
    {
        return _colormap_sample_lut(colormap->lut, colormap->lut_count, t, out_rgba);
    }

    if (colormap != NULL)
    {
        uint32_t builtin_count = 0;
        const DvzColormapStop* builtin =
            _colormap_builtin_stops(colormap->builtin, &builtin_count);
        if (builtin != NULL && builtin_count >= 2)
            return _colormap_sample_stops(builtin, builtin_count, t, out_rgba);
    }

    uint8_t gray = (uint8_t)(255.0 * t + 0.5);
    out_rgba[0] = gray;
    out_rgba[1] = gray;
    out_rgba[2] = gray;
    out_rgba[3] = 255;
    return true;
}



/**
 * Sample a scene-owned colormap at a normalized coordinate.
 *
 * @param colormap the colormap, or NULL for grayscale fallback
 * @param t normalized scalar coordinate
 * @param out the output RGBA color
 * @return true when a color was written
 */
bool dvz_colormap_sample(const DvzColormap* colormap, double t, DvzColor* out)
{
    ANN(out);
    uint8_t rgba[4] = {0};
    const bool ok = _scene_color_from_colormap(colormap, t, rgba);
    *out = dvz_color_rgba(rgba[0], rgba[1], rgba[2], rgba[3]);
    return ok;
}



/**
 * Sample a built-in colormap at a normalized coordinate.
 *
 * @param builtin the built-in colormap selector
 * @param t normalized scalar coordinate
 * @param out the output RGBA color
 * @return true when a color was written
 */
bool dvz_colormap_builtin_sample(DvzBuiltinColormap builtin, double t, DvzColor* out)
{
    ANN(out);
    DvzColormap colormap = {
        .kind = DVZ_COLORMAP_CONTINUOUS,
        .builtin = builtin,
    };
    return dvz_colormap_sample(&colormap, t, out);
}


/**
 * Create a scene-owned colormap object.
 *
 * @param scene the scene
 * @param desc the colormap descriptor, or NULL for defaults
 * @return the colormap, or NULL on allocation failure
 */
DvzColormap* dvz_colormap(DvzScene* scene, const DvzColormapDesc* desc)
{
    ANN(scene);
    if (!_colormap_desc_validate(desc))
        return NULL;
    if (scene->colormap_count >= DVZ_SCENE_MAX_COLORMAPS)
    {
        log_error("maximum colormap count reached");
        return NULL;
    }
    DvzColormap* colormap = &scene->colormaps[scene->colormap_count++];
    dvz_memset(colormap, sizeof(DvzColormap), 0, sizeof(DvzColormap));
    colormap->scene = scene;
    colormap->kind = desc != NULL ? desc->kind : DVZ_COLORMAP_CONTINUOUS;
    colormap->builtin = desc != NULL ? desc->builtin : DVZ_BUILTIN_COLORMAP_NONE;
    if (desc != NULL)
    {
        colormap->center = desc->center;
        colormap->has_center = desc->center != 0.0;
        if (desc->label != NULL)
            dvz_strlcpy(colormap->label, desc->label, sizeof(colormap->label));
    }
    return colormap;
}



/**
 * Create a scene-owned built-in colormap object.
 *
 * @param scene the scene
 * @param builtin the built-in colormap selector
 * @return the colormap, or NULL on allocation failure
 */
DvzColormap* dvz_colormap_builtin(DvzScene* scene, DvzBuiltinColormap builtin)
{
    DvzColormapDesc desc = dvz_colormap_desc();
    desc.builtin = builtin;
    return dvz_colormap(scene, &desc);
}


/**
 * Create a scene-owned custom LUT colormap.
 *
 * @param scene the scene
 * @param label optional colormap label
 * @param colors RGBA8 lookup table
 * @param count number of colors in the lookup table
 * @return the colormap, or NULL on error
 */
DvzColormap* dvz_colormap_custom(
    DvzScene* scene, const char* label, const DvzColor* colors, uint32_t count)
{
    ANN(scene);
    if (colors == NULL || count < 2)
    {
        log_error("custom colormap requires at least two colors");
        return NULL;
    }
    const size_t count_size = (size_t)count;
    if (count_size > SIZE_MAX / sizeof(DvzColor))
    {
        log_error("custom colormap LUT size overflow");
        return NULL;
    }

    const size_t size = count_size * sizeof(DvzColor);
    DvzColor* lut = (DvzColor*)dvz_calloc(count_size, sizeof(DvzColor));
    if (lut == NULL)
    {
        log_error("custom colormap LUT allocation failed");
        return NULL;
    }
    dvz_memcpy(lut, size, colors, size);

    DvzColormap* colormap = dvz_colormap(
        scene, &(DvzColormapDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzColormapDesc),
                   .kind = DVZ_COLORMAP_CONTINUOUS,
                   .builtin = DVZ_BUILTIN_COLORMAP_NONE,
                   .label = label,
               });
    if (colormap == NULL)
    {
        dvz_free(lut);
        return NULL;
    }

    colormap->lut = lut;
    colormap->lut_count = count;
    return colormap;
}



/**
 * Destroy a colormap object.
 *
 * @param colormap the colormap
 */
void dvz_colormap_destroy(DvzColormap* colormap)
{
    if (colormap == NULL)
        return;
    _colormap_release_lut(colormap);
    colormap->scene = NULL;
    colormap->stop_count = 0;
    colormap->has_center = false;
}



/**
 * Set custom color stops on a colormap.
 *
 * @param colormap the colormap
 * @param stops the color stops
 * @param count the number of stops
 */
void dvz_colormap_set_stops(DvzColormap* colormap, const DvzColormapStop* stops, uint32_t count)
{
    ANN(colormap);
    if (count > DVZ_SCENE_MAX_COLOR_STOPS)
    {
        log_error("too many color stops: %u > %u", count, DVZ_SCENE_MAX_COLOR_STOPS);
        return;
    }
    if (count > 0)
        ANN(stops);
    _colormap_release_lut(colormap);
    colormap->stop_count = count;
    if (count > 0)
        dvz_memcpy(
            colormap->stops, sizeof(colormap->stops), stops, count * sizeof(DvzColormapStop));
    _scene_mark_colormap_dirty(colormap);
}



/**
 * Set the diverging center on a colormap.
 *
 * @param colormap the colormap
 * @param center the semantic center value
 */
void dvz_colormap_set_center(DvzColormap* colormap, double center)
{
    ANN(colormap);
    colormap->center = center;
    colormap->has_center = true;
    _scene_mark_colormap_dirty(colormap);
}
