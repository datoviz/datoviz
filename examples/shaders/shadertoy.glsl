// Created by inigo quilez - 2020
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0

// Other "Iterations" shaders:
//
// "trigonometric"   : https://www.shadertoy.com/view/Mdl3RH
// "trigonometric 2" : https://www.shadertoy.com/view/Wss3zB
// "circles"         : https://www.shadertoy.com/view/MdVGWR
// "coral"           : https://www.shadertoy.com/view/4sXGDN
// "guts"            : https://www.shadertoy.com/view/MssGW4
// "inversion"       : https://www.shadertoy.com/view/XdXGDS
// "inversion 2"     : https://www.shadertoy.com/view/4t3SzN
// "shiny"           : https://www.shadertoy.com/view/MslXz8
// "worms"           : https://www.shadertoy.com/view/ldl3W4
// "stripes"         : https://www.shadertoy.com/view/wlsfRn

#define AA 2

float hash( in float n )
{
    return fract(sin(n)*43758.5453);
}
float noise( in vec2 p )
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f*f*(3.0-2.0*f);
    float n = i.x + i.y*57.0;
    return mix(mix( hash(n+ 0.0), hash(n+ 1.0),f.x),
               mix( hash(n+57.0), hash(n+58.0),f.x),f.y);
}

vec2 map( in vec2 p, in float time )
{
    for( int i=0; i<4; i++ )
    {
    	float a = noise(p*1.5)*6.2831 + time;
		p += 0.1*vec2( cos(a), sin(a) );
    }
    return p;
}

float height( in vec2 p, in vec2 q )
{
    float h = dot(p-q,p-q);
    h += 0.005*texture(iChannel0,0.75*(p+q)).x;
    return h;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float time = 0.25*iTime;

    vec3 tot = vec3(0.0);
	#if AA>1
    for( int m=0; m<AA; m++ )
    for( int n=0; n<AA; n++ )
    {
        vec2 o = vec2(float(m),float(n)) / float(AA) - 0.5;
        vec2 p = (2.0*(fragCoord+o)-iResolution.xy)/iResolution.y;
		#else
        vec2 p = (2.0*fragCoord-iResolution.xy)/iResolution.y;
		#endif

        // deformation
        vec2 q = map(p,time);

        // color
        float w = 10.0*q.x;
        float u = floor(w);
        float f = fract(w);
        vec3  col = vec3(0.7,0.55,0.5) + 0.3*sin(3.0*u+vec3(0.0,1.5,2.0));

        // filtered drop-shadow
        float sha = smoothstep(0.0,0.5,f)-smoothstep(1.0-fwidth(w),1.0,f);

        // normal
        vec2  eps = vec2(2.0/iResolution.y,0.0);
		float l2c = height(q,p);
        float l2x = height(map(p+eps.xy,time),p) - l2c;
        float l2y = height(map(p+eps.yx,time),p) - l2c;
        vec3  nor = normalize( vec3( l2x, eps.x, l2y ) );

        // lighting
        col *= 0.3+0.7*sha;
        col *= 0.8+0.2*vec3(1.0,0.9,0.3)*dot(nor,vec3(0.7,0.3,0.7));
        col += 0.3*pow(nor.y,8.0)*sha;
        col *= 7.5*l2c;

        tot += col;
	#if AA>1
    }
    tot /= float(AA*AA);
	#endif

	fragColor = vec4( tot, 1.0 );
}
