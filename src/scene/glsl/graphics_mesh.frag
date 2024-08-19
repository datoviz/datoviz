#version 450
#include "common.glsl"
#include "params_mesh.glsl"

float edgeFactor(vec3 barycentric, float linewidth)
{
    vec3 d = fwidth(barycentric);
    // vec3 a3 = smoothstep(vec3(0.0), d * 50, barycentric);
    vec3 a3 = step(d * linewidth, barycentric);
    return min(min(a3.x, a3.y), a3.z);
}

const float eps = .00001;

// Varying variables.
layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_uvcolor;
layout(location = 3) in vec3 in_barycentric;

layout(location = 0) out vec4 out_color;

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex;

void main()
{

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


    // Texture.
    if (MESH_TEXTURED > 0)
    {
        // in this case, in_uvcolor.xy is uv coordinates
        color = texture(tex, in_uvcolor.xy).xyz;
    }
    // Color.
    else
    {
        color = in_uvcolor.xyz; // rgb
    }

    // Lighting.
    if (MESH_LIGHTING > 0)
    {
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
        diff = max(diff, max(dot(light_dir, -normal), 0.0));
        diffuse = diff * light_color;

        // Specular component.
        view_dir = normalize(-mvp.view[3].xyz - in_pos);
        reflect_dir = reflect(-light_dir, normal);
        spec = pow(max(dot(view_dir, reflect_dir), 0.0), lpar.w);
        specular = spec * light_color;

        // Total color.
        out_color.xyz += (lpar.x * ambient + lpar.y * diffuse + lpar.z * specular) * color;
        out_color.a =
            in_uvcolor.a; // by convention, alpha channel is in 4th component of this attr
    }
    else
    {
        // NOTE: the 4th component of in_uvcolor is always the alpha channel, both in the
        // color case (rgba) or the uv tex case (uv*a).
        out_color = vec4(color, in_uvcolor.a);
    }

    // Stroke.
    if (params.stroke.a > 0)
    {
        float e = edgeFactor(in_barycentric, params.stroke.a);
        out_color.rgb = mix(params.stroke.rgb, out_color.rgb, e);
    }
}
