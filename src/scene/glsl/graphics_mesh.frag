#version 450
#include "common.glsl"

#include "params_mesh.glsl"

// layout(binding = (USER_BINDING + 1)) uniform sampler2D tex;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_color;
// layout(location = ) in vec2 in_uv;
// layout(location = ) in float in_clip;
// layout(location = ) in float in_alpha;

layout(location = 0) out vec4 out_color;

const float eps = .00001;

void main()
{
    out_color = in_color;

    CLIP;

    // if (in_clip < -eps)
    //     discard;

    vec3 normal, light_dir, ambient, diffuse, view_dir, reflect_dir, specular, color;
    vec4 lpar;
    vec3 lpos;
    vec3 light_color = vec3(1, 1, 1); // TODO: customizable light color
    float diff, spec;

    normal = normalize(in_normal);
    out_color = vec4(0, 0, 0, 1);
    diffuse = vec3(0);
    specular = vec3(0);

    // Color.
    color = in_color.xyz;
    // if (in_uv.y >= 0)
    // {
    //     // color += params.tex_coefs.x * texture(tex_0, in_uv).xyz;
    // }

    // Light position and params.
    lpos = params.light_pos.xyz;
    lpar = params.light_params;

    // Light direction.
    light_dir = normalize(lpos - in_pos);

    // Ambient component.
    ambient = light_color;

    // Diffuse component.
    // HACK: normals on both faces
    diff = max(dot(light_dir, normal), 0.0);
    // diff = max(diff, max(dot(light_dir, -normal), 0.0));
    diffuse = diff * light_color;

    // Specular component.
    view_dir = normalize(-mvp.view[3].xyz - in_pos);
    // view_dir = normalize(-in_pos);
    reflect_dir = reflect(-light_dir, normal);
    spec = pow(max(dot(view_dir, reflect_dir), 0.0), lpar.w);
    specular = spec * light_color;

        // Total color.
        out_color.xyz += (lpar.x * ambient + lpar.y * diffuse + lpar.z * specular) * color;
    }

    out_color.a = 1.0; // in_alpha;
}
