const float eps = 1e-5;


float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}


float contour(float d, float w) {
    return smoothstep(0.5 - w, 0.5 + w, d);
}


float get_alpha(vec2 uv) {
    vec2 msdf_unit = 4.0 / params.tex_size;
    vec4 smp = texture(tex_sampler, tex_coords);
    float sig_dist = median(smp.r, smp.g, smp.b) - 0.5;
    sig_dist *= dot(msdf_unit, 0.5 / fwidth(uv));
    sig_dist += 0.5;
    return clamp(sig_dist, 0.0, 1.0);
}


float samp(vec2 uv, float w) {
    return contour(get_alpha(uv), w);
}


float supersample(float alpha) {
    // from http://www.java-gaming.org/index.php?PHPSESSID=lvd34ig10qe05pgvq3lj3rh8a4&topic=33612.msg316185#msg316185
    float width = fwidth(alpha);
    // Supersample, 4 extra points
    float dscale = 0.354; // half of 1/sqrt2; you can play with this
    vec2 duv = dscale * (dFdx(tex_coords) + dFdy(tex_coords));
    vec4 box = vec4(tex_coords - duv, tex_coords + duv);
    float asum = samp(box.xy, width)
               + samp(box.zw, width)
               + samp(box.xw, width)
               + samp(box.zy, width);
    alpha = (alpha + 0.5 * asum) / 3.0;

    return alpha;
}
