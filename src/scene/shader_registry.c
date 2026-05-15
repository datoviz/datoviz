/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene shader registry                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_shader_registry.h"

#include "datoviz/drp2/stream.h"
#include "datoviz/fileio/fileio.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DRP2_VERTEX_WGSL                                                                        \
    "@vertex fn main() -> @builtin(position) vec4f { return vec4f(0.0, 0.0, 0.0, 1.0); }"
#define DRP2_FULLSCREEN_VERTEX_WGSL                                                             \
    "@vertex fn main(@builtin(vertex_index) idx: u32) -> @builtin(position) vec4f { var pos = " \
    "array<vec2f, 3>(vec2f(-1.0, -1.0), vec2f(3.0, -1.0), vec2f(-1.0, 3.0)); return "          \
    "vec4f(pos[idx], 0.0, 1.0); }"
#define DRP2_FRAGMENT_WGSL                                                                      \
    "@fragment fn main() -> @location(0) vec4f { return vec4f(1.0, 1.0, 1.0, 1.0); }"
#define DRP2_TEXTURE_VERTEX_WGSL                                                                \
    "@vertex fn main(@builtin(vertex_index) idx: u32) -> @builtin(position) vec4f { var pos = " \
    "array<vec2f, 3>(vec2f(-1.0, -1.0), vec2f(3.0, -1.0), vec2f(-1.0, 3.0)); return "          \
    "vec4f(pos[idx], 0.0, 1.0); }"
#define DRP2_TEXTURE_FRAGMENT_WGSL                                                              \
    "@group(0) @binding(0) var source: texture_2d<f32>; @group(0) @binding(1) var samp: "      \
    "sampler; @fragment fn main(@builtin(position) pos: vec4f) -> @location(0) vec4f { "       \
    "return textureSample(source, samp, vec2f(0.5, 0.5)); }"
#define DRP2_COMPUTE_WGSL                                                                       \
    "@group(0) @binding(0) var<storage, read> input: array<f32>; @group(0) @binding(1) "        \
    "var<storage, read_write> output: array<f32>; @compute @workgroup_size(9) fn main("        \
    "@builtin(global_invocation_id) id: vec3u) { output[id.x] = input[id.x]; }"
#define DRP2_PIXEL_VERTEX_WGSL                                                                  \
    "struct MVP { model: mat4x4f, view: mat4x4f, proj: mat4x4f, time: f32, flags: u32, }\n"    \
    "struct Viewport { rect: vec4f, }\n"                                                        \
    "struct VertexIn { @location(0) position: vec3f, @location(1) color: vec4f, "               \
    "@location(2) size: f32, }\n"                                                               \
    "struct VertexOut { @builtin(position) position: vec4f, @location(0) color: vec4f, }\n"     \
    "@group(0) @binding(0) var<uniform> mvp: MVP;\n"                                            \
    "@group(0) @binding(1) var<uniform> viewport: Viewport;\n"                                  \
    "fn quad_corner(vertex_id: u32) -> vec2f {"                                                  \
    "let corners = array<vec2f, 6>(vec2f(-1.0, -1.0), vec2f(1.0, -1.0), "                      \
    "vec2f(-1.0, 1.0), vec2f(-1.0, 1.0), vec2f(1.0, -1.0), vec2f(1.0, 1.0));"                 \
    "return corners[vertex_id];}\n"                                                            \
    "@vertex fn main(@builtin(vertex_index) vertex_id: u32, input: VertexIn) -> VertexOut {"    \
    "let corner = quad_corner(vertex_id);"                                                      \
    "let center = mvp.proj * mvp.view * mvp.model * vec4f(input.position, 1.0);"                \
    "let radius = vec2f(input.size / viewport.rect.z, input.size / viewport.rect.w);"           \
    "var output: VertexOut;"                                                                    \
    "output.position = vec4f(center.xy + corner * radius * center.w, center.zw);"               \
    "output.color = input.color; return output;}\n"
#define DRP2_PIXEL_FRAGMENT_WGSL                                                                \
    "struct FragmentIn { @location(0) color: vec4f, }\n"                                       \
    "@fragment fn main(input: FragmentIn) -> @location(0) vec4f { return input.color; }\n"

#define DRP2_VERTEX_GLSL                                                                        \
    "#version 450\nvoid main(){gl_Position=vec4(0.0,0.0,0.0,1.0);}"
#define DRP2_FULLSCREEN_VERTEX_GLSL                                                             \
    "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"                       \
    "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"
#define DRP2_FRAGMENT_GLSL                                                                      \
    "#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1.0);}"
#define DRP2_TEXTURE_VERTEX_GLSL                                                                \
    "#version 450\nvec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));"                       \
    "void main(){gl_Position=vec4(p[gl_VertexIndex],0,1);}"
#define DRP2_TEXTURE_FRAGMENT_GLSL                                                              \
    "#version 450\nlayout(set=0,binding=0)uniform sampler2D t;"                                \
    "layout(location=0)out vec4 c;void main(){c=texture(t,vec2(.5));}"
#define DRP2_COMPUTE_GLSL                                                                       \
    "#version 450\nlayout(local_size_x=1)in;void main(){}"

/* Point visual: separate vertex buffers for position (vec3), color (u8 RGBA->vec4), size (float). */
#define DRP2_POINT_VERTEX_GLSL                                                                  \
    "#version 450\n"                                                                            \
    "layout(set=0,binding=0)uniform MVP{mat4 model;mat4 view;mat4 proj;float time;uint flags;}mvp;\n" \
    "layout(set=0,binding=1)uniform Viewport{vec4 rect;}viewport;\n"                           \
    "layout(location=0)in vec3 inPos;\n"                                                        \
    "layout(location=1)in vec4 inColor;\n"                                                      \
    "layout(location=2)in float inSize;\n"                                                      \
    "layout(location=0)out vec4 fragColor;\n"                                                   \
    "void main(){"                                                                               \
    "gl_Position=mvp.proj*mvp.view*mvp.model*vec4(inPos,1.0);"                                 \
    "gl_PointSize=inSize;"                                                                       \
    "fragColor=inColor;}\n"
#define DRP2_POINT_FRAGMENT_GLSL                                                                \
    "#version 450\n"                                                                            \
    "layout(location=0)in vec4 fragColor;\n"                                                    \
    "layout(location=0)out vec4 outColor;\n"                                                    \
    "void main(){outColor=fragColor;}\n"
#define DRP2_PIXEL_VERTEX_GLSL DRP2_POINT_VERTEX_GLSL
#define DRP2_PIXEL_FRAGMENT_GLSL DRP2_POINT_FRAGMENT_GLSL
#define DRP2_POINT_PICK_VERTEX_GLSL                                                             \
    "#version 450\n"                                                                            \
    "layout(set=0,binding=0)uniform MVP{mat4 model;mat4 view;mat4 proj;float time;uint flags;}mvp;\n" \
    "layout(set=0,binding=1)uniform Viewport{vec4 rect;}viewport;\n"                           \
    "layout(location=0)in vec3 inPos;\n"                                                        \
    "layout(location=2)in float inSize;\n"                                                      \
    "layout(location=0)flat out uint fragId;\n"                                                 \
    "void main(){"                                                                               \
    "gl_Position=mvp.proj*mvp.view*mvp.model*vec4(inPos,1.0);"                                 \
    "gl_PointSize=inSize;"                                                                       \
    "fragId=uint(gl_VertexIndex)+1u;}\n"
#define DRP2_POINT_PICK_FRAGMENT_GLSL                                                           \
    "#version 450\n"                                                                            \
    "layout(location=0)flat in uint fragId;\n"                                                  \
    "layout(location=0)out vec4 outColor;\n"                                                    \
    "void main(){outColor=vec4(float(fragId&255u)/255.0,float((fragId>>8u)&255u)/255.0,"       \
    "float((fragId>>16u)&255u)/255.0,float((fragId>>24u)&255u)/255.0);}\n"

/* Primitive visual: position (vec3) + color (u8 RGBA->vec4); topology selected per visual. */
#define DRP2_PRIMITIVE_VERTEX_GLSL                                                              \
    "#version 450\n"                                                                            \
    "layout(set=0,binding=0)uniform MVP{mat4 model;mat4 view;mat4 proj;float time;uint flags;}mvp;\n" \
    "layout(set=0,binding=1)uniform Viewport{vec4 rect;}viewport;\n"                           \
    "layout(location=0)in vec3 inPos;\n"                                                        \
    "layout(location=1)in vec4 inColor;\n"                                                      \
    "layout(location=0)out vec4 fragColor;\n"                                                   \
    "void main(){gl_Position=mvp.proj*mvp.view*mvp.model*vec4(inPos,1.0);fragColor=inColor;}\n"
#define DRP2_PRIMITIVE_FRAGMENT_GLSL                                                            \
    "#version 450\n"                                                                            \
    "layout(location=0)in vec4 fragColor;\n"                                                    \
    "layout(location=0)out vec4 outColor;\n"                                                    \
    "void main(){outColor=fragColor;}\n"
#define DRP2_PRIMITIVE_VERTEX_WGSL                                                              \
    "struct MVP { model: mat4x4f, view: mat4x4f, proj: mat4x4f, time: f32, flags: u32, }\n"    \
    "struct Viewport { rect: vec4f, }\n"                                                        \
    "struct VertexIn { @location(0) position: vec3f, @location(1) color: vec4f, }\n"            \
    "struct VertexOut { @builtin(position) position: vec4f, @location(0) color: vec4f, }\n"     \
    "@group(0) @binding(0) var<uniform> mvp: MVP;\n"                                            \
    "@group(0) @binding(1) var<uniform> viewport: Viewport;\n"                                  \
    "fn transform(position: vec3f) -> vec4f {"                                                   \
    "return mvp.proj * mvp.view * mvp.model * vec4f(position, 1.0);}\n"                         \
    "@vertex fn main(input: VertexIn) -> VertexOut {"                                           \
    "var output: VertexOut; output.position = transform(input.position);"                       \
    "output.color = input.color; return output;}\n"
#define DRP2_PRIMITIVE_FRAGMENT_WGSL                                                            \
    "struct FragmentIn { @location(0) color: vec4f, }\n"                                       \
    "@fragment fn main(input: FragmentIn) -> @location(0) vec4f { return input.color; }\n"
#define DRP2_PRIMITIVE_LIT_VERTEX_GLSL                                                          \
    "#version 450\n"                                                                            \
    "layout(set=0,binding=0)uniform MVP{mat4 model;mat4 view;mat4 proj;float time;uint flags;}mvp;\n" \
    "layout(set=0,binding=1)uniform Viewport{vec4 rect;}viewport;\n"                           \
    "layout(location=0)in vec3 inPos;\n"                                                        \
    "layout(location=1)in vec4 inColor;\n"                                                      \
    "layout(location=2)in vec3 inNormal;\n"                                                     \
    "layout(location=0)out vec4 fragColor;\n"                                                   \
    "layout(location=1)out vec3 fragNormal;\n"                                                  \
    "layout(location=2)out vec3 fragWorldPos;\n"                                                \
    "layout(location=3)out vec3 fragCameraPos;\n"                                               \
    "vec4 transform(vec3 pos){vec4 tr=mvp.proj*mvp.view*mvp.model*vec4(pos,1.0);"              \
    "tr.y=-tr.y;tr.z=0.5*(tr.z+tr.w);return tr;}\n"                                            \
    "void main(){vec4 world=mvp.model*vec4(inPos,1.0);gl_Position=transform(inPos);"           \
    "fragColor=inColor;fragWorldPos=world.xyz;"                                                \
    "fragCameraPos=(inverse(mvp.view)*vec4(0,0,0,1)).xyz;"                                     \
    "fragNormal=transpose(inverse(mat3(mvp.model)))*inNormal;}\n"
#define DRP2_PRIMITIVE_LIT_FRAGMENT_GLSL                                                        \
    "#version 450\n"                                                                            \
    "layout(set=1,binding=0)uniform PrimitiveShading{vec4 lightDir;vec4 params;}shading;\n"    \
    "layout(location=0)in vec4 fragColor;\n"                                                    \
    "layout(location=1)in vec3 fragNormal;\n"                                                   \
    "layout(location=2)in vec3 fragWorldPos;\n"                                                 \
    "layout(location=3)in vec3 fragCameraPos;\n"                                                \
    "layout(location=0)out vec4 outColor;\n"                                                    \
    "void main(){vec3 n=normalize(fragNormal);vec3 l=normalize(shading.lightDir.xyz);"         \
    "vec3 v=normalize(fragCameraPos-fragWorldPos);vec3 h=normalize(l+v);"                      \
    "float lambert=max(dot(n,l),0.0);float spec=pow(max(dot(n,h),0.0),32.0);"                  \
    "vec3 rgb=fragColor.rgb*(shading.params.x+shading.params.y*lambert)+vec3(0.18*spec);"     \
    "outColor=vec4(clamp(rgb,0.0,1.0),fragColor.a);}\n"

/* Image visual: position (vec3) + texcoords (vec2); samples a 2D RGBA8 texture. */
#define DRP2_IMAGE_VERTEX_GLSL                                                                  \
    "#version 450\n"                                                                            \
    "layout(set=0,binding=0)uniform MVP{mat4 model;mat4 view;mat4 proj;float time;uint flags;}mvp;\n" \
    "layout(set=0,binding=1)uniform Viewport{vec4 rect;}viewport;\n"                           \
    "layout(location=0)in vec3 inPos;\n"                                                        \
    "layout(location=1)in vec2 inUV;\n"                                                         \
    "layout(location=0)out vec2 fragUV;\n"                                                      \
    "vec4 transform(vec3 pos){vec4 tr=mvp.proj*mvp.view*mvp.model*vec4(pos,1.0);"              \
    "tr.y=-tr.y;tr.z=0.5*(tr.z+tr.w);return tr;}\n"                                            \
    "void main(){gl_Position=transform(inPos);fragUV=inUV;}\n"
#define DRP2_IMAGE_FRAGMENT_GLSL                                                                \
    "#version 450\n"                                                                            \
    "layout(set=1,binding=0)uniform sampler2D tex;\n"                                           \
    "layout(location=0)in vec2 fragUV;\n"                                                       \
    "layout(location=0)out vec4 outColor;\n"                                                    \
    "void main(){outColor=texture(tex,fragUV);}\n"
#define DRP2_WBOIT_ACCUM_FRAGMENT_GLSL                                                          \
    "#version 450\n"                                                                            \
    "layout(location=0)in vec4 fragColor;\n"                                                    \
    "layout(location=0)out vec4 outAccum;\n"                                                     \
    "layout(location=1)out float outWeight;\n"                                                   \
    "void main(){float a=clamp(fragColor.a,0.0,1.0);"                                          \
    "outAccum=vec4(fragColor.rgb*a,a);outWeight=a;}\n"
#define DRP2_WBOIT_ACCUM_LIT_FRAGMENT_GLSL                                                      \
    "#version 450\n"                                                                            \
    "layout(set=1,binding=0)uniform PrimitiveShading{vec4 lightDir;vec4 params;}shading;\n"    \
    "layout(location=0)in vec4 fragColor;\n"                                                    \
    "layout(location=1)in vec3 fragNormal;\n"                                                   \
    "layout(location=2)in vec3 fragWorldPos;\n"                                                 \
    "layout(location=3)in vec3 fragCameraPos;\n"                                                \
    "layout(location=0)out vec4 outAccum;\n"                                                     \
    "layout(location=1)out float outWeight;\n"                                                   \
    "void main(){vec3 n=normalize(fragNormal);if(!gl_FrontFacing)n=-n;"                        \
    "vec3 l=normalize(shading.lightDir.xyz);"                                                   \
    "vec3 v=normalize(fragCameraPos-fragWorldPos);vec3 h=normalize(l+v);"                      \
    "float lambert=max(dot(n,l),0.0);float spec=pow(max(dot(n,h),0.0),32.0);"                  \
    "vec3 rgb=fragColor.rgb*(shading.params.x+shading.params.y*lambert)+vec3(0.18*spec);"     \
    "float a=clamp(fragColor.a,0.0,1.0);vec3 lit=clamp(rgb,0.0,1.0);"                          \
    "outAccum=vec4(lit*a,a);outWeight=a;}\n"
#define DRP2_WBOIT_RESOLVE_FRAGMENT_GLSL                                                        \
    "#version 450\n"                                                                            \
    "layout(set=0,binding=0)uniform texture2D accumTex;\n"                                      \
    "layout(set=0,binding=1)uniform texture2D weightTex;\n"                                     \
    "layout(set=0,binding=2)uniform sampler samp;\n"                                            \
    "layout(location=0)out vec4 outColor;\n"                                                     \
    "void main(){vec2 uv=gl_FragCoord.xy/vec2(textureSize(sampler2D(accumTex,samp),0));"        \
    "vec4 accum=texture(sampler2D(accumTex,samp),uv);"                                          \
    "float weight=texture(sampler2D(weightTex,samp),uv).r;"                                     \
    "float alpha=clamp(accum.a,0.0,1.0);"                                                       \
    "vec3 rgb=weight>1e-5?accum.rgb/max(weight,1e-5):vec3(0.0);"                               \
    "outColor=vec4(rgb,alpha);}\n"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the short shader-format tag used in runtime cache keys.
 *
 * @param cfg the emission configuration
 * @return "g" for GLSL, "w" otherwise
 */
const char* _shader_format_tag(const DvzFramePlanEmitConfig* cfg)
{
    if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return "g";
    return "w";
}



/**
 * Return the DRP2 shader-format token for a scene fixture emission config.
 *
 * @param cfg the emission config
 * @return the shader-format token
 */
const char* _shader_format_token(const DvzFramePlanEmitConfig* cfg)
{
    if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return "glsl";
    return "wgsl";
}



/**
 * Select shader source for a scene fixture emission config.
 *
 * @param cfg the emission config
 * @param wgsl the WGSL shader source
 * @param glsl the GLSL shader source
 * @return the selected shader source
 */
const char*
_shader_source(const DvzFramePlanEmitConfig* cfg, const char* wgsl, const char* glsl)
{
    if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return glsl;
    return wgsl;
}



/**
 * Return the vertex shader source used for simple render emissions.
 *
 * @param cfg the emission config
 * @param wgsl output WGSL shader source
 * @param glsl output GLSL shader source
 */
void _render_vertex_shader_source(
    const DvzFramePlanEmitConfig* cfg, const char** wgsl, const char** glsl)
{
    if (cfg != NULL && cfg->fullscreen_triangle)
    {
        *wgsl = DRP2_FULLSCREEN_VERTEX_WGSL;
        *glsl = DRP2_FULLSCREEN_VERTEX_GLSL;
    }
    else
    {
        *wgsl = DRP2_VERTEX_WGSL;
        *glsl = DRP2_VERTEX_GLSL;
    }
}



/**
 * Emit a shader module command with the configured source format.
 *
 * @param stream the DRP2 stream
 * @param id the shader id
 * @param stage the shader stage
 * @param wgsl the WGSL shader source
 * @param glsl the GLSL shader source
 * @param cfg the emission config
 * @return whether the command was appended
 */
bool _emit_shader(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage, const char* wgsl,
    const char* glsl, const DvzFramePlanEmitConfig* cfg)
{
    return dvz_drp2_stream_create_shader_module_format(
        stream, id, stage, _shader_format_token(cfg), _shader_source(cfg, wgsl, glsl));
}



/**
 * Emit a shader using precompiled SPIR-V when available, or runtime GLSL otherwise.
 *
 * @param stream the DRP2 stream
 * @param id the shader id
 * @param stage the shader stage
 * @param spirv_key the embedded SPIR-V resource key
 * @param glsl the GLSL fallback source
 * @param cfg the emission config
 * @return whether the command was appended
 */
bool _emit_shader_spirv(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage,
    const char* spirv_key, const char* glsl, const DvzFramePlanEmitConfig* cfg)
{
    (void)cfg;
    unsigned long spv_size = 0;
    const unsigned char* spv = dvz_resource_shader(spirv_key, &spv_size);
    if (spv != NULL && spv_size > 0)
        return dvz_drp2_stream_create_shader_module_spirv(
            stream, id, stage, spv, (uint64_t)spv_size);
    return dvz_drp2_stream_create_shader_module_format(stream, id, stage, "glsl", glsl);
}



/**
 * Return the fallback GLSL source for a built-in scene shader.
 *
 * @param shader the built-in shader key
 * @param fragment whether to return the fragment stage variant
 * @return the GLSL source, or NULL when the key/stage pair is invalid
 */
const char* _builtin_shader_glsl(DvzSceneBuiltinShader shader, bool fragment)
{
    switch (shader)
    {
    case DVZ_SCENE_BUILTIN_SHADER_FIXTURE:
        return fragment ? DRP2_FRAGMENT_GLSL : DRP2_VERTEX_GLSL;
    case DVZ_SCENE_BUILTIN_SHADER_TEXTURE:
        return fragment ? DRP2_TEXTURE_FRAGMENT_GLSL : DRP2_TEXTURE_VERTEX_GLSL;
    case DVZ_SCENE_BUILTIN_SHADER_COMPUTE_COPY:
        return fragment ? NULL : DRP2_COMPUTE_GLSL;
    case DVZ_SCENE_BUILTIN_SHADER_POINT:
        return fragment ? DRP2_POINT_FRAGMENT_GLSL : DRP2_POINT_VERTEX_GLSL;
    case DVZ_SCENE_BUILTIN_SHADER_PIXEL:
        return fragment ? DRP2_PIXEL_FRAGMENT_GLSL : DRP2_PIXEL_VERTEX_GLSL;
    case DVZ_SCENE_BUILTIN_SHADER_POINT_PICK:
        return fragment ? DRP2_POINT_PICK_FRAGMENT_GLSL : DRP2_POINT_PICK_VERTEX_GLSL;
    case DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE:
        return fragment ? DRP2_PRIMITIVE_FRAGMENT_GLSL : DRP2_PRIMITIVE_VERTEX_GLSL;
    case DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT:
        return fragment ? DRP2_PRIMITIVE_LIT_FRAGMENT_GLSL : DRP2_PRIMITIVE_LIT_VERTEX_GLSL;
    case DVZ_SCENE_BUILTIN_SHADER_IMAGE:
        return fragment ? DRP2_IMAGE_FRAGMENT_GLSL : DRP2_IMAGE_VERTEX_GLSL;
    case DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM:
        return fragment ? DRP2_WBOIT_ACCUM_FRAGMENT_GLSL : DRP2_PRIMITIVE_VERTEX_GLSL;
    case DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM_LIT:
        return fragment ? DRP2_WBOIT_ACCUM_LIT_FRAGMENT_GLSL : DRP2_PRIMITIVE_LIT_VERTEX_GLSL;
    case DVZ_SCENE_BUILTIN_SHADER_WBOIT_RESOLVE:
        return fragment ? DRP2_WBOIT_RESOLVE_FRAGMENT_GLSL : DRP2_FULLSCREEN_VERTEX_GLSL;
    default:
        return NULL;
    }
}


/**
 * Return the WGSL source for a built-in scene shader.
 *
 * @param shader the built-in shader key
 * @param fragment whether to return the fragment stage variant
 * @return the WGSL source, or NULL when the key/stage pair has no WGSL variant
 */
const char* _builtin_shader_wgsl(DvzSceneBuiltinShader shader, bool fragment)
{
    switch (shader)
    {
    case DVZ_SCENE_BUILTIN_SHADER_POINT:
#if DVZ_HAS_WGSL_SHADERS
    {
        unsigned long size = 0;
        const char* key = fragment ? "point_frag" : "point_vert";
        const char* source = dvz_resource_wgsl(key, &size);
        if (source != NULL && size > 0)
            return source;
    }
#endif
        return NULL;
    case DVZ_SCENE_BUILTIN_SHADER_PIXEL:
#if DVZ_HAS_WGSL_SHADERS
    {
        unsigned long size = 0;
        const char* key = fragment ? "pixel_frag" : "pixel_vert";
        const char* source = dvz_resource_wgsl(key, &size);
        if (source != NULL && size > 0)
            return source;
    }
#endif
        return fragment ? DRP2_PIXEL_FRAGMENT_WGSL : DRP2_PIXEL_VERTEX_WGSL;
    case DVZ_SCENE_BUILTIN_SHADER_IMAGE:
#if DVZ_HAS_WGSL_SHADERS
    {
        unsigned long size = 0;
        const char* key = fragment ? "image_frag" : "image_vert";
        const char* source = dvz_resource_wgsl(key, &size);
        if (source != NULL && size > 0)
            return source;
    }
#endif
        return NULL;
    case DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE:
#if DVZ_HAS_WGSL_SHADERS
    {
        unsigned long size = 0;
        const char* key = fragment ? "primitive_frag" : "primitive_vert";
        const char* source = dvz_resource_wgsl(key, &size);
        if (source != NULL && size > 0)
            return source;
    }
#endif
        return fragment ? DRP2_PRIMITIVE_FRAGMENT_WGSL : DRP2_PRIMITIVE_VERTEX_WGSL;
    default:
        return NULL;
    }
}



/**
 * Return the fixture vertex WGSL source.
 *
 * @return the WGSL source
 */
const char* _fixture_vertex_wgsl(void)
{
    return DRP2_VERTEX_WGSL;
}



/**
 * Return the fullscreen triangle WGSL source.
 *
 * @return the WGSL source
 */
const char* _fullscreen_vertex_wgsl(void)
{
    return DRP2_FULLSCREEN_VERTEX_WGSL;
}



/**
 * Return the fixture fragment WGSL source.
 *
 * @return the WGSL source
 */
const char* _fixture_fragment_wgsl(void)
{
    return DRP2_FRAGMENT_WGSL;
}



/**
 * Return the texture vertex WGSL source.
 *
 * @return the WGSL source
 */
const char* _texture_vertex_wgsl(void)
{
    return DRP2_TEXTURE_VERTEX_WGSL;
}



/**
 * Return the texture fragment WGSL source.
 *
 * @return the WGSL source
 */
const char* _texture_fragment_wgsl(void)
{
    return DRP2_TEXTURE_FRAGMENT_WGSL;
}



/**
 * Return the compute-copy WGSL source.
 *
 * @return the WGSL source
 */
const char* _compute_copy_wgsl(void)
{
    return DRP2_COMPUTE_WGSL;
}
