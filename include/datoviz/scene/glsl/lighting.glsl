/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Basic Lighting                                                                               */
/*************************************************************************************************/
/*

    A minimal lighting model that is meant to be small and fast.

    Lights:
        Has only a single light. (For Now.)
        The light can eiher be a position or a direction.

    Material:
        Each material property (ambient, diffuse, specular) acts on rgb colors components
        to the same degree.
        Material properties to be provided in shader as a single vec4.
        vec4 material = vec4(ambient, diffuse, specular, shininess);
*/


struct BasicLight {
    vec4 pos;
    vec4 color;
};

//  The material is from a vec3 uniform.

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


/*************************************************************************************************/
/*  Advance Lighting                                                                        */
/*************************************************************************************************/

/*
    Lights:

        Up to 4 lights in the scene at once.

        A non zero alpha component indicates the light is on.
        Alpha determines the brightness.

        A light postion with w set to zero indicates it's a direction with no position.
        Set w to 1.0 initially for positioned lights.

        Additional attributes are used to make lights act more realistic in the scene.

        mat4x4 <-- one vec4 for each light

            mat4 pos     x,y,z,w     w>0 is position.  w=0 is direction.
            mat4 color   r,b,b,a     a>0 indicates light is present. (usually 1.0)
            mat4 atts    (power, fov, size, range)  <-- Not implemented yet.


    Material:

        Material properties define how lights react to the object.

        A material can react to red differently than blue.
        This can be used to model objects more realistically with different light conditions.

        One mat4 material is used with all lights.

        mat4x4 <-- 4 vec4
           0  ambient       r,g,b,-
           1  diffuse       r,g,b,-
           2  specular      r,g,b,-
           3  emmission     r,g,b,-

        float shine     The shininess of the surface.
        float emit      The imission amount.
*/


struct Light {
    mat4 pos;        // w=0 indicates it's a direction with no position.
    mat4 color;      // alpha value indicates it's on.
};


struct Material {
    mat4 params;          /* (R, G, B, -)  X  (ambient, diffuse, specualar, emission) */
    float shine;          /* specular amount */
    float emit;           /* emission level */
};


vec4 lighting(vec4 pos, vec4 color, vec3 normal, vec4 cam_pos, Light light, Material material)
{
    vec3 N = normalize(normal);                     // Surface angle to camera
    vec3 CD = normalize(cam_pos.xyz - pos.xyz);     // Camera Direction

    vec4 total_color = vec4(0.0, 0.0, 0.0, 1.0);

    // Ambient and emission do not depend on lights.
    vec4 ambient = material.params[0] * (0.5 + 0.5 * N.z);

    int count = 0;
    for (int i=0; i<4; i++)
    {
        if (light.color[i].a == 0.0)
            continue;                       // This light is not visible.

        // Light direction.
        vec3 LD;
        if (light.pos[i].w == 0)
            LD = normalize(light.pos[i].xyz);          // is a light direction.
        else
            LD = normalize(light.pos[i].xyz - pos.xyz);    // is a light position.

        float diffuse = max(dot(N, LD), 0.0);

        // Blinn-Phong
        vec3 HA = normalize(LD + CD);              // Half Angle
        float specular = pow(max(dot(N, HA), 0.0), 1.0 + material.shine * 128.0);

        // Color
        vec4 C = diffuse * material.params[1];
        C += specular * material.params[2] * (0.5 + material.shine * 3.0);
        total_color += C * light.color[i];
        count++;
    }
    if (count>0) {
        total_color /= count;
    }
    total_color += ambient * color;
    total_color += material.emit * material.params[3];
    total_color = min(total_color, 1.0);
    total_color.a = 1.0;
    return total_color;
}
