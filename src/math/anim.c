/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Animation                                                                                    */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>

#include "_log.h"
#include "datoviz/math/anim.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Easing functions                                                                             */
/*************************************************************************************************/

// from: https://raw.githubusercontent.com/nicolausYes/easing-functions/master/src/easing.cpp
// see also: https://easings.net

static double easeInSine(double t) { return 1 - cos(DVZ_PI_2 * t); }

static double easeOutSine(double t) { return sin(DVZ_PI_2 * t); }

static double easeInOutSine(double t) { return 0.5 * (1 + sin(DVZ_PI * (t - 0.5))); }

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
    return t2 * t2 * sin(t * DVZ_PI * 4.5);
}

static double easeOutElastic(double t)
{
    double t2 = (t - 1) * (t - 1);
    return 1 - t2 * t2 * cos(t * DVZ_PI * 4.5);
}

static double easeInOutElastic(double t)
{
    double t2;
    if (t < 0.45)
    {
        t2 = t * t;
        return 8 * t2 * t2 * sin(t * DVZ_PI * 9);
    }
    else if (t < 0.55)
    {
        return 0.5 + 0.75 * sin(t * DVZ_PI * 4);
    }
    else
    {
        t2 = (t - 1) * (t - 1);
        return 1 - 8 * t2 * t2 * sin(t * DVZ_PI * 9);
    }
}

static double easeInBounce(double t) { return pow(2, 6 * (t - 1)) * fabs(sin(t * DVZ_PI * 3.5)); }

static double easeOutBounce(double t) { return 1 - pow(2, -6 * t) * fabs(cos(t * DVZ_PI * 3.5)); }

static double easeInOutBounce(double t)
{
    if (t < 0.5)
    {
        return 8 * pow(2, 8 * (t - 1)) * fabs(sin(t * DVZ_PI * 7));
    }
    else
    {
        return 1 - 8 * pow(2, -8 * t) * fabs(sin(t * DVZ_PI * 7));
    }
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
