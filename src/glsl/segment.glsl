    // Viewport coordinates.
    mat4 ortho = get_ortho_matrix(viewport.size);
    mat4 ortho_inv = inverse(ortho);

    vec4 p0 = ortho_inv * P0_;
    vec4 p1 = ortho_inv * P1_;

    // NOTE: we need to normalize by the homogeneous coordinates after converting into pixels.
    p0.xyz /= p0.w;
    p1.xyz /= p1.w;

    float z = p0.z;

    vec2 position = p0.xy;
    vec2 T = (p1 - p0).xy;
    out_length = length(T);
    float w = linewidth / 2.0 + 1.5 * antialias;
    T = w * normalize(T);

    if (index < 0.5) {
       position = vec2(p0.x - T.y - T.x, p0.y + T.x - T.y);
       out_texcoord = vec2(-w, +w);
       z = p0.z;
       out_cap = cap0;
    }
    else if (index < 1.5) {
       position = vec2(p0.x + T.y - T.x, p0.y - T.x - T.y);
       out_texcoord = vec2(-w, -w);
       z = p0.z;
       out_cap = cap0;
    }
    else if (index < 2.5) {
       position = vec2(p1.x + T.y + T.x, p1.y - T.x + T.y);
       out_texcoord = vec2(out_length + w, -w);
       z = p1.z;
       out_cap = cap1;
    }
    else {
       position = vec2(p1.x - T.y + T.x, p1.y + T.x + T.y);
       out_texcoord = vec2(out_length + w, +w);
       z = p1.z;
       out_cap = cap1;
    }

    gl_Position = ortho * vec4(position, z, 1.0);
