
    // Size of one glyph in NDC.
    float w = 2 * glyph_size.x;
    float h = 2 * glyph_size.y;

    // Which vertex within the triangle strip forming the rectangle.
    int i = gl_VertexIndex % 4;

    // Rectangle vertex displacement (one glyph = one rectangle = 6 vertices)
    float dx = int(i / 2.0);
    float dy = mod(i, 2.0);

    // Position of the glyph.
    vec2 origin = .5 * vec2(glyph.z * w, h) * (anchor - 1);
    vec2 p = origin + vec2(w * glyph.y, 0);

    // gl_Position = pos_tr;
    gl_Position = ortho_inv * pos_tr;
    gl_Position.xy += gl_Position.w * rotation * (p + vec2(dx * w, dy * h));  // bottom left of the glyph
    gl_Position = ortho * gl_Position;

    // Index in the texture
    float rows = params.grid_size.x;
    float cols = params.grid_size.y;

    float row = floor(glyph.x / cols);
    float col = mod(glyph.x, cols);

    // uv position in the texture for the glyph.
    vec2 uv = vec2(col, row);
    uv /= vec2(cols, rows);

    // Little margin to avoid edge effects between glyphs.
    float eps = .005;
    dx = eps + (1.0 - 2 * eps) * dx;
    dy = eps + (1.0 - 2 * eps) * dy;

    // Texture coordinates for the fragment shader.
    vec2 duv = vec2(dx / cols, dy /rows);

    // Output variables.
    out_tex_coords = uv + duv;

    // String index, used to discard between different strings.
    out_str_index = float(glyph.w);
