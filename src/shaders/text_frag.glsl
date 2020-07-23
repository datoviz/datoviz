if (fract(str_index + eps) > 2 * eps) discard;

float alpha = get_alpha(tex_coords);
alpha = supersample(alpha);
out_color = color;
out_color.a *= alpha;
