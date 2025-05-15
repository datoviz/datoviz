/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */



/* ==========================================================================
    Lighting Rendering Models:

        Basic             A Single light with vec4 material properties, fast, and small.
                          (As currently used by spheres.)
                          Red, green, and blue light components are treated equally.

        Advanced          Has up to 4 lights, mat4 properties, is more realistic materials.
                          May be somewhat slower than basic and use more memory.
                          Material can react differently to red, green, and blue light.

        * Realistic       Physical Based rendering with less concern for performance.

        * RayTrace_fast   Attempt to be fast enough to render in real time, but may be
                          limited to low resolutions and quality.

        * RayTrace_full   Attempt to produce high quality production level visuals/images.
                          May be very slow.

    (*) Not implemented yet.
============================================================================= */


/*
    Basic Lighting:

        Basic lighting is meant to be a simpler model that is both small and fast.

        Has only a single light. (For Now.)

        Each material property (ambient, diffuse, specular) acts on rgb colors components
        to the same degree.

        * attributes for power,fov,area, and attenuation not implemented yet.
*/


// not used yet... Should this be in include directory?
//layout(std140, binding = USER_BINDING) uniform BasicLightParams
//{
//    vec4 pos;              // default = camera position
//    vec4 color;            // default = white
//    vec4 params;
//    // vec4 atts;             // power, fov, area, attenuation.   (This may change.)
//}
//light_params;


// Material properties to be provided in shader as a uniform vec4.
//
// vec4 material = vec4(ambient, diffuse, specular, shininess);
//


vec4 basic_lighting(vec4 pos, vec4 color, vec4 material, vec3 normal,
                    vec4 cam_pos, vec4 light_pos, vec4 light_color)
{
    vec3 N = normalize(normal);
    vec3 CD = normalize(cam_pos.xyz - pos.xyz);     // Camera Direction

        // Light direction.
        vec3 LD;
        if (light_pos.w == 0)
            LD = normalize(light_pos.xyz);          // is a light direction.
        else
            LD = normalize(light_pos.xyz - pos.xyz);    // is a light position.

    // Corrections for vulkan y direction.   *** Move outside lighting function ***
    // Postion data from vertex shader after model matrix transform.  (World Space.)
    //LD.y *= -1.0;
    //CD.y *= -1.0;

    // Ambient when near 0.0,  Emission when near 1.0.
    float ambient = mix(material.x, 2.0 * N.z, material.x * material.x);

    float diffuse = material.y * max(dot(N, LD), 0.0);

    // Blinn-Phong
    vec3 HA = normalize(CD + LD);    // Half Angle
    float spec_term = pow(max(dot(N, HA), 0.0), 1.0 + material.w * 128.0);
    float specular = 2.0 * material.z * material.z * spec_term;

    // Color
    vec4 C = min(ambient * color, 1.0);
    C += (1.0 - C) * diffuse * color * light_color;
    C += (1.0 - C) * specular * light_color;
    C.a = color.a;

    return C;
}



/*
    Advanced Lighting:

        Up to 4 lights in the scene at once.

        A non zero alpha component indicates the light is on.
        Alpha determines the brightness.
        A light postion with w set to zero indicates it's a direction with no position.
        Set w to 1.0 initially for positioned lights.

        Additional attributes are used to make lights act more realistic in the scene.
*/
//layout(std140, binding = USER_BINDING) uniform AdvanceLightParams
//{
//    // Up to 4 lights.
//    mat4 color;        // (0, 1, 2, 3) x (r, g, b, a)   a=0 is off or absent.
//    mat4 pos;          // (0, 1, 2, 3) x (x, y, z, w)   w=0 indicates it's a direction.
//    mat4 atts;         // (0, 1, 2, 3) x (fov, power, area, attenuation)  Proposed, may change.
//}
//params;

/*
    Advance Material properties defines how lihgts react to the object.

    A material can react to red differently than blue.
    This can be used to model objects more realistically with different light conditions.

    One mat4 material is used with all lights.  (Not one per light.)

    mat4x4 <-- 4 vec3
       0  ambient       r,g,b,-
       1  diffuse       r,g,b,-
       2  specular      r,g,b,-
       3  shininess     r,g,b,-
*/
// uniform mat4 adv_material;  // (ambient, diffuse, specular, shininess) X (r,g,b,a)


vec4 advanced_lighting(vec4 pos, vec4 material_color, mat4 material, vec3 normal,
                       vec4 cam_pos, mat4 light_pos, mat4 light_color)
{
    vec3 N = normalize(normal);             // Surface angle to camera
    vec3 CD = normalize(cam_pos.xyz - pos.xyz);     // Camera Direction

    vec4 total_color = vec4(0.0, 0.0, 0.0, 1.0);

    // Ambient light doesn't depend on lights.
    // Ambient when near 0.0,  Emission when near 1.0.
    vec4 ambient = mix(material[0], vec4(2.0 * N.z), material[0] * material[0]);

    int count = 0;
    for (int i=0; i<4; i++)
    {
        if (light_color[i].a == 0.0)
            continue;                       // This light is not visible.

        // Light direction.
        vec3 LD;
        if (light_pos[i].w == 0)
            LD = normalize(light_pos[i].xyz);          // is a light direction.
        else
            LD = normalize(light_pos[i].xyz - pos.xyz);    // is a light position.

        vec4 diffuse = material[1] * max(dot(N, LD), 0.0);

        // Blinn-Phong
        vec3 HA = normalize(LD + CD);              // Half Angle
        vec4 spec_term = pow(vec4(max(dot(N, HA), 0.0)), 1.0 + material[3] * 128.0);
        vec4 specular = 2.0 * material[2] * material[2] * spec_term;

        // Color
        vec4 color = diffuse * material_color * light_color[i];
        color += specular * light_color[i];

        total_color += color;
        count++;
    }
    total_color /= count;
    total_color += ambient * material_color;
    total_color = min(total_color, 1.0);
    total_color.a = 1.0;
    return total_color;
}
