#version 450
// #include "../../src/shaders/common.glsl"

/*
Reference : glumpy "mandelbrot.py" example
*/

layout (location = 0) in vec2 in_coords;

layout (location = 0) out vec4 out_color;

vec3 hot(float t) {
    return vec3(smoothstep(0.00, 0.33, t),
                smoothstep(0.33, 0.66, t),
                smoothstep(0.66, 1.00, t));
}

void main()
{
    const int n = 300;
    const float log_2 = 0.6931471805599453;
    vec2 c = 3.0 * (.5 * (1 + in_coords)) - vec2(2.0,1.5);

    float x, y, d;
    int i;
    vec2 z = c;
    for(i = 0; i < n; ++i)
    {
        x = (z.x*z.x - z.y*z.y) + c.x;
        y = (z.y*z.x + z.x*z.y) + c.y;
        d = x*x + y*y;
        if (d > 4.0) break;
        z = vec2(x,y);
    }

    if ( i < n ) {
        float nu = log(log(sqrt(d))/log_2)/log_2;
        float index = float(i) + 1.0 - nu;
        float v = pow(index/float(n),0.5);
        out_color = vec4(hot(v), 1.0);
    } else {
        out_color = vec4(hot(0.0), 1.0);
    }

}
