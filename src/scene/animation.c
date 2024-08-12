/*************************************************************************************************/
/*  Animation                                                                                    */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_cglm.h"
#include "_log.h"
#include "datoviz.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Easing functions                                                                             */
/*************************************************************************************************/

// from: https://raw.githubusercontent.com/nicolausYes/easing-functions/master/src/easing.cpp
// see also: https://easings.net

static double easeInSine(double t) { return sin(M_PI2 * t); }

static double easeOutSine(double t) { return sin(M_PI2 * t); }

static double easeInOutSine(double t) { return 0.5 * (1 + sin(M_PI * (t - 0.5))); }

static double easeInQuad(double t) { return t * t; }

static double easeOutQuad(double t) { return t * (2 - t); }

static double easeInOutQuad(double t) { return t < 0.5 ? 2 * t * t : t * (4 - 2 * t) - 1; }

static double easeInCubic(double t) { return t * t * t; }

static double easeOutCubic(double t)
{
    double u = 1 - t;
    return 1 - u * u * u;
}

static double easeInOutCubic(double t)
{
    double u = -2 * t + 2;
    return t < 0.5 ? 4 * t * t * t : 1 - u * u * u / 2;
}

static double easeInQuart(double t)
{
    t *= t;
    return t * t;
}

static double easeOutQuart(double t)
{
    double u = 1 - t;
    u *= u;
    return 1 - u * u;
}

static double easeInOutQuart(double t)
{
    double u = -2 * t + 2;
    return t < 0.5 ? 8 * t * t * t * t : 1 - u * u * u * u / 2;
}

static double easeInQuint(double t)
{
    double t2 = t * t;
    return t * t2 * t2;
}

static double easeOutQuint(double t)
{
    double u = 1 - t;
    return 1 - u * u * u * u * u;
}

static double easeInOutQuint(double t)
{
    double u = -2 * t + 2;
    return t < 0.5 ? 16 * t * t * t * t * t : 1 - u * u * u * u * u / 2;
}

static double easeInExpo(double t) { return (pow(2, 8 * t) - 1) / 255; }

static double easeOutExpo(double t) { return t == 1 ? 1 : 1 - pow(2, -10 * t); }

static double easeInOutExpo(double t)
{
    return t == 0    ? 0
           : t == 1  ? 1
           : t < 0.5 ? pow(2, 20 * t - 10) / 2
                     : (2 - pow(2, -20 * t + 10)) / 2;
}

static double easeInCirc(double t) { return 1 - sqrt(1 - t); }

static double easeOutCirc(double t) { return sqrt(t); }

static double easeInOutCirc(double t)
{
    if (t < 0.5)
    {
        return (1 - sqrt(1 - 2 * t)) * 0.5;
    }
    else
    {
        return (1 + sqrt(2 * t - 1)) * 0.5;
    }
}

static double easeInBack(double t) { return t * t * (2.70158 * t - 1.70158); }

static double easeOutBack(double t)
{
    const double c1 = 1.70158;
    const double c3 = c1 + 1;
    double u = t - 1;
    return 1 + c3 * u * u * u + c1 * u * u;
}

static double easeInOutBack(double t)
{
    const double c1 = 1.70158;
    const double c2 = c1 * 1.525;
    return t < 0.5 ? (pow(2 * t, 2) * ((c2 + 1) * 2 * t - c2)) / 2
                   : (pow(2 * t - 2, 2) * ((c2 + 1) * (t * 2 - 2) + c2) + 2) / 2;
}

static double easeInElastic(double t)
{
    double t2 = t * t;
    return t2 * t2 * sin(t * M_PI * 4.5);
}

static double easeOutElastic(double t)
{
    double t2 = (t - 1) * (t - 1);
    return 1 - t2 * t2 * cos(t * M_PI * 4.5);
}

static double easeInOutElastic(double t)
{
    double t2;
    if (t < 0.45)
    {
        t2 = t * t;
        return 8 * t2 * t2 * sin(t * M_PI * 9);
    }
    else if (t < 0.55)
    {
        return 0.5 + 0.75 * sin(t * M_PI * 4);
    }
    else
    {
        t2 = (t - 1) * (t - 1);
        return 1 - 8 * t2 * t2 * sin(t * M_PI * 9);
    }
}

static double easeInBounce(double t) { return pow(2, 6 * (t - 1)) * fabs(sin(t * M_PI * 3.5)); }

static double easeOutBounce(double t) { return 1 - pow(2, -6 * t) * fabs(cos(t * M_PI * 3.5)); }

static double easeInOutBounce(double t)
{
    if (t < 0.5)
    {
        return 8 * pow(2, 8 * (t - 1)) * fabs(sin(t * M_PI * 7));
    }
    else
    {
        return 1 - 8 * pow(2, -8 * t) * fabs(sin(t * M_PI * 7));
    }
}



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static void orthonormal_basis(vec3 axis, vec3 u, vec3 v, vec3 w)
{
    // Construct an orthonormal basis around an axis vector.
    // from: https://stackoverflow.com/a/7753446/1595060

    if (glm_vec3_norm(axis) < EPSILON)
    {
        log_error(
            "norm of input vector {%f, %f, %f} is too small to compute an orthonormal basis.",
            axis[0], axis[1], axis[2]);
        return;
    }

    // Let's call a your unit vector.
    vec3 a = {0};
    glm_normalize_to(axis, a);

    // Call u0 = (1,0,0).
    vec3 u0 = {1, 0, 0};

    // If dot(u0,a) ~= 0, then take u0 = (0,1,0).
    if (fabs(a[0]) < EPSILON)
    {
        u0[0] = 0;
        u0[1] = 1;
    }

    // Then, v = a ^ u0
    glm_vec3_crossn(a, u0, v);

    // and w = a ^ v.
    glm_vec3_crossn(a, v, w);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

double dvz_easing(DvzEasing easing, double t)
{
    switch (easing)
    {
    case DVZ_EASING_NONE:
        return t;
    case DVZ_EASING_IN_SINE:
        return easeInSine(t);
        break;
    case DVZ_EASING_OUT_SINE:
        return easeOutSine(t);
        break;
    case DVZ_EASING_IN_OUT_SINE:
        return easeInOutSine(t);
        break;
    case DVZ_EASING_IN_QUAD:
        return easeInQuad(t);
        break;
    case DVZ_EASING_OUT_QUAD:
        return easeOutQuad(t);
        break;
    case DVZ_EASING_IN_OUT_QUAD:
        return easeInOutQuad(t);
        break;
    case DVZ_EASING_IN_CUBIC:
        return easeInCubic(t);
        break;
    case DVZ_EASING_OUT_CUBIC:
        return easeOutCubic(t);
        break;
    case DVZ_EASING_IN_OUT_CUBIC:
        return easeInOutCubic(t);
        break;
    case DVZ_EASING_IN_QUART:
        return easeInQuart(t);
        break;
    case DVZ_EASING_OUT_QUART:
        return easeOutQuart(t);
        break;
    case DVZ_EASING_IN_OUT_QUART:
        return easeInOutQuart(t);
        break;
    case DVZ_EASING_IN_QUINT:
        return easeInQuint(t);
        break;
    case DVZ_EASING_OUT_QUINT:
        return easeOutQuint(t);
        break;
    case DVZ_EASING_IN_OUT_QUINT:
        return easeInOutQuint(t);
        break;
    case DVZ_EASING_IN_EXPO:
        return easeInExpo(t);
        break;
    case DVZ_EASING_OUT_EXPO:
        return easeOutExpo(t);
        break;
    case DVZ_EASING_IN_OUT_EXPO:
        return easeInOutExpo(t);
        break;
    case DVZ_EASING_IN_CIRC:
        return easeInCirc(t);
        break;
    case DVZ_EASING_OUT_CIRC:
        return easeOutCirc(t);
        break;
    case DVZ_EASING_IN_OUT_CIRC:
        return easeInOutCirc(t);
        break;
    case DVZ_EASING_IN_BACK:
        return easeInBack(t);
        break;
    case DVZ_EASING_OUT_BACK:
        return easeOutBack(t);
        break;
    case DVZ_EASING_IN_OUT_BACK:
        return easeInOutBack(t);
        break;
    case DVZ_EASING_IN_ELASTIC:
        return easeInElastic(t);
        break;
    case DVZ_EASING_OUT_ELASTIC:
        return easeOutElastic(t);
        break;
    case DVZ_EASING_IN_OUT_ELASTIC:
        return easeInOutElastic(t);
        break;
    case DVZ_EASING_IN_BOUNCE:
        return easeInBounce(t);
        break;
    case DVZ_EASING_OUT_BOUNCE:
        return easeOutBounce(t);
        break;
    case DVZ_EASING_IN_OUT_BOUNCE:
        return easeInOutBounce(t);
        break;
    default:
        break;
    }
    log_warn("easing %d is not implemented", (int)easing);
    return t;
}



// Resample from [t0, t1] to [0, 1].
double dvz_resample(double t0, double t1, double t) { return (t - t0) / (t1 - t0); }



void dvz_circular_2D(vec2 center, float radius, float angle, float t, vec2 out)
{
    float a = M_2PI * t + angle;
    vec2 u = {cos(a), sin(a)};
    out[0] = center[0] + radius * u[0];
    out[1] = center[1] + radius * u[1];
}



void dvz_circular_3D(vec3 center, vec3 u, vec3 v, float radius, float angle, float t, vec3 out)
{
    vec2 center_p = {0};
    center_p[0] = center[0];
    center_p[1] = center[1];

    vec2 out_p = {0};
    dvz_circular_2D(center_p, radius, angle, t, out_p);
    float x = out_p[0];
    float y = out_p[1];

    out[0] = center[0] + u[0] * x + v[0] * y;
    out[1] = center[1] + u[1] * x + v[1] * y;
    out[2] = center[2] + u[2] * x + v[2] * y;
}



// interpolation between p0 and p1, t in [0, 1]
float dvz_interpolate(float p0, float p1, float t) { return p0 + p1 * t; }



void dvz_interpolate_2D(vec2 p0, vec2 p1, float t, vec2 out) { glm_vec2_lerp(p0, p1, t, out); }



void dvz_interpolate_3D(vec3 p0, vec3 p1, float t, vec3 out) { glm_vec3_lerp(p0, p1, t, out); }
