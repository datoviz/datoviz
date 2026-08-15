/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text fonts                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "datoviz/scene.h"
#include "text_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define DVZ_FONT_DESC_KNOWN_FLAGS 0u


static bool _font_desc_validate(const DvzFontDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(desc, DvzFontDesc, DVZ_FONT_DESC_KNOWN_FLAGS))
    {
        log_error("invalid font descriptor ABI");
        return false;
    }
    return true;
}


/**
 * Return the scene default SDF font, creating it lazily.
 *
 * @param scene the scene
 * @return the default font, or NULL on allocation failure
 */
static DvzFont* _text_default_sdf_font(DvzScene* scene)
{
    ANN(scene);
    DvzFontDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzFontDesc),
        .path = scene->font_defaults.sans_path,
        .family = scene->font_defaults.sans_family,
        .style = scene->font_defaults.sans_style,
        .face_index = scene->font_defaults.sans_face_index,
        .font_flags = scene->font_defaults.sans_font_flags,
    };
    if (desc.family == NULL || desc.family[0] == '\0')
        desc.family = "Source Sans 3";
    if (desc.style == NULL || desc.style[0] == '\0')
        desc.style = "Regular";
    for (uint32_t i = 0; i < scene->font_count; i++)
    {
        bool path_matches = false;
        if (desc.path == NULL || desc.path[0] == '\0')
            path_matches = scene->fonts[i].path[0] == '\0';
        else
            path_matches = strcmp(scene->fonts[i].path, desc.path) == 0;

        if (
            path_matches && strcmp(scene->fonts[i].family, desc.family) == 0 &&
            strcmp(scene->fonts[i].style, desc.style) == 0)
            return &scene->fonts[i];
    }
    return dvz_font(scene, &desc);
}



/**
 * Resolve the font used by an SDF text style.
 *
 * @param scene the scene
 * @param style the text style
 * @return a scene-owned font, or NULL on failure
 */
DvzFont* _text_sdf_font(DvzScene* scene, const DvzTextStyle* style)
{
    ANN(scene);
    ANN(style);
    if (style->font != NULL)
        return style->font;
    return _text_default_sdf_font(scene);
}


/**
 * Return the font-owned atlas for a requested spec after ensure has run.
 *
 * @param font the font
 * @param spec requested atlas spec
 * @return the resolved atlas, including fallback atlases
 */
DvzTextAtlas* _text_font_atlas(DvzFont* font, const DvzTextAtlasSpec* spec)
{
    ANN(font);
    ANN(spec);
    return _scene_text_atlas_get(font, spec);
}





/**
 * Return a compact version sum for all scene font resources.
 *
 * @param scene the scene
 * @return the summed font version value
 */
uint64_t _text_scene_font_version_sum(const DvzScene* scene)
{
    ANN(scene);
    uint64_t version = 0;
    for (uint32_t i = 0; i < scene->font_count; i++)
        version += scene->fonts[i].version;
    return version;
}






/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a scene-owned font resource.
 *
 * @param scene the scene
 * @param desc the font descriptor
 * @return the font, or NULL on allocation failure
 */
DvzFont* dvz_font(DvzScene* scene, const DvzFontDesc* desc)
{
    ANN(scene);
    ANN(desc);
    if (!_font_desc_validate(desc))
        return NULL;
    if (scene->font_count >= DVZ_SCENE_MAX_FONTS)
    {
        log_error("maximum font count reached");
        return NULL;
    }
    DvzFont* font = &scene->fonts[scene->font_count++];
    dvz_memset(font, sizeof(DvzFont), 0, sizeof(DvzFont));
    font->scene = scene;
    font->face_index = desc->face_index;
    font->flags = desc->font_flags;
    font->version = 1;
    if (desc->path != NULL)
        dvz_strlcpy(font->path, desc->path, sizeof(font->path));
    if (desc->family != NULL)
        dvz_strlcpy(font->family, desc->family, sizeof(font->family));
    if (desc->style != NULL)
        dvz_strlcpy(font->style, desc->style, sizeof(font->style));
    return font;
}



/**
 * Destroy a scene-owned font resource.
 *
 * @param font the font
 */
void dvz_font_destroy(DvzFont* font)
{
    if (font == NULL)
        return;
    _scene_font_release(font);
}
