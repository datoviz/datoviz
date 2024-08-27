#version 450
#include "common.glsl"
#include "params_mesh.glsl"

const float eps = .00001;

// Varying variables.
layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_uvcolor;
layout(location = 3) in vec3 in_barycentric;
layout(location = 4) in vec3 in_d_left;
layout(location = 5) in vec3 in_d_right;
layout(location = 6) flat in ivec3 in_contour;

layout(location = 0) out vec4 out_color;

layout(binding = (USER_BINDING + 1)) uniform sampler2D tex;

// Vertex corner between the left and right edge.
float one_corner(float d_left, float d_right, float linewidth)
{
    vec2 d = abs(vec2(d_left, d_right));
    vec2 deltas = fwidth(d);  // rate of change of the distances
    float a = d.x / deltas.x; // normalized distance to left edge
    float b = d.y / deltas.y; // normalized distance to right edge
    // NOTE: the sign of d_left/right indicates the orientation.
    float in_orient = d_left > 0 ? 0 : 1;
    float c = in_orient > 0 ? min(a, b) : max(a, b); // take min or max of the distance
    return smoothstep(linewidth, linewidth + 1, c);  // 0 on contour, 1 inside the polygon
}

float corner(vec3 d_left, vec3 d_right, ivec3 contour, float linewidth)
{
    bool corner_x = ((contour.x >> 1) & 1) > 0;
    bool corner_y = ((contour.y >> 1) & 1) > 0;
    bool corner_z = ((contour.z >> 1) & 1) > 0;

    float res = 1;
    if (corner_x)
        res = min(res, one_corner(d_left.x, d_right.x, linewidth));
    if (corner_y)
        res = min(res, one_corner(d_left.y, d_right.y, linewidth));
    if (corner_z)
        res = min(res, one_corner(d_left.z, d_right.z, linewidth));
    return res;
}

float edge(vec3 barycentric, ivec3 contour, float linewidth)
{
    // cf https://web.archive.org/web/20190220052115/http://codeflow.org/entries/2012/aug/02/
    // easy-wireframe-display-with-barycentric-coordinates/
    // cf https://catlikecoding.com/unity/tutorials/advanced-rendering/flat-and-wireframe-shading/

    vec3 deltas = fwidth(barycentric);
    float scale = linewidth * 0.5;
    vec3 a = deltas * scale;
    vec3 b = deltas * (scale + 1);

    vec3 stepped = smoothstep(a, b, barycentric);
    float x = stepped.x;
    float y = stepped.y;
    float z = stepped.z;

    bool edge_x = ((contour.x >> 0) & 1) > 0;
    bool edge_y = ((contour.y >> 0) & 1) > 0;
    bool edge_z = ((contour.z >> 0) & 1) > 0;

    float res = 1;
    if (edge_x)
        res = min(res, x);
    if (edge_y)
        res = min(res, y);
    if (edge_z)
        res = min(res, z);
    return res;
}



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
        float linewidth = params.stroke.a;
        vec3 stroke = params.stroke.rgb;
        float e = edge(in_barycentric, in_contour, linewidth);
        float c = corner(in_d_left, in_d_right, in_contour, linewidth);
        out_color.rgb = mix(stroke, out_color.rgb, min(e, c));
    }
}
