#version 450
layout(location = 0) out vec4 out_color;
layout(location = 0) in flat uint in_id;
void main() {
    float shift = float(in_id & 3u) * 0.1;
    out_color = vec4(1.0, 0.5 + shift, 0.0, 1.0);
}
