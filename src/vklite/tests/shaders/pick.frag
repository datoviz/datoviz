#version 450
layout(location = 0) out uint out_id;
layout(location = 0) in flat uint in_id;
void main() {
    out_id = in_id;
}
