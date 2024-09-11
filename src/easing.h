#ifndef EASING_H
#define EASING_H

#ifdef __cplusplus
extern "C" {
#endif

// Linear Easing
float easeLinear(float x);

// Quadratic Easing
float easeInQuad(float x);
float easeOutQuad(float x);
float easeInOutQuad(float x);

// Cubic Easing
float easeInCubic(float x);
float easeOutCubic(float x);
float easeInOutCubic(float x);

// Quartic Easing
float easeInQuart(float x);
float easeOutQuart(float x);
float easeInOutQuart(float x);

// Quintic Easing
float easeInQuint(float x);
float easeOutQuint(float x);
float easeInOutQuint(float x);

// Sine Easing
float easeInSine(float x);
float easeOutSine(float x);
float easeInOutSine(float x);

// Exponential Easing
float easeInExpo(float x);
float easeOutExpo(float x);
float easeInOutExpo(float x);

// Circular Easing
float easeInCirc(float x);
float easeOutCirc(float x);
float easeInOutCirc(float x);

// Bounce Easing
float easeInBounce(float x);
float easeOutBounce(float x);
float easeInOutBounce(float x);

// Elastic Easing
float easeInElastic(float x);
float easeOutElastic(float x);
float easeInOutElastic(float x);

// Back Easing
float easeInBack(float x);
float easeOutBack(float x);
float easeInOutBack(float x);

#ifdef __cplusplus
}
#endif

#endif // EASING_H

// Implementation
#ifdef EASING_IMPLEMENTATION

#include <math.h>

// Linear Easing
float easeLinear(float x) {
    return x;
}

// Quadratic Easing
float easeInQuad(float x) {
    return x * x;
}

float easeOutQuad(float x) {
    return 1 - (1 - x) * (1 - x);
}

float easeInOutQuad(float x) {
    return x < 0.5 ? 2 * x * x : 1 - powf(-2 * x + 2, 2) / 2;
}

// Cubic Easing
float easeInCubic(float x) {
    return x * x * x;
}

float easeOutCubic(float x) {
    return 1 - powf(1 - x, 3);
}

float easeInOutCubic(float x) {
    return x < 0.5 ? 4 * x * x * x : 1 - powf(-2 * x + 2, 3) / 2;
}

// Quartic Easing
float easeInQuart(float x) {
    return x * x * x * x;
}

float easeOutQuart(float x) {
    return 1 - powf(1 - x, 4);
}

float easeInOutQuart(float x) {
    return x < 0.5 ? 8 * x * x * x * x : 1 - powf(-2 * x + 2, 4) / 2;
}

// Quintic Easing
float easeInQuint(float x) {
    return x * x * x * x * x;
}

float easeOutQuint(float x) {
    return 1 - powf(1 - x, 5);
}

float easeInOutQuint(float x) {
    return x < 0.5 ? 16 * x * x * x * x * x : 1 - powf(-2 * x + 2, 5) / 2;
}

// Sine Easing
float easeInSine(float x) {
    return 1 - cosf((x * M_PI) / 2);
}

float easeOutSine(float x) {
    return sinf((x * M_PI) / 2);
}

float easeInOutSine(float x) {
    return -(cosf(M_PI * x) - 1) / 2;
}

// Exponential Easing
float easeInExpo(float x) {
    return x == 0 ? 0 : powf(2, 10 * x - 10);
}

float easeOutExpo(float x) {
    return x == 1 ? 1 : 1 - powf(2, -10 * x);
}

float easeInOutExpo(float x) {
    if (x == 0) return 0;
    if (x == 1) return 1;
    return x < 0.5 ? powf(2, 20 * x - 10) / 2 : (2 - powf(2, -20 * x + 10)) / 2;
}

// Circular Easing
float easeInCirc(float x) {
    return 1 - sqrtf(1 - powf(x, 2));
}

float easeOutCirc(float x) {
    return sqrtf(1 - powf(x - 1, 2));
}

float easeInOutCirc(float x) {
    return x < 0.5 ? (1 - sqrtf(1 - powf(2 * x, 2))) / 2 : (sqrtf(1 - powf(-2 * x + 2, 2)) + 1) / 2;
}

// Bounce Easing
float easeInBounce(float x) {
    return 1 - easeOutBounce(1 - x);
}

float easeOutBounce(float x) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;

    if (x < 1 / d1) {
        return n1 * x * x;
    } else if (x < 2 / d1) {
        x -= 1.5f / d1;
        return n1 * x * x + 0.75f;
    } else if (x < 2.5f / d1) {
        x -= 2.25f / d1;
        return n1 * x * x + 0.9375f;
    } else {
        x -= 2.625f / d1;
        return n1 * x * x + 0.984375f;
    }
}

float easeInOutBounce(float x) {
    return x < 0.5 ? (1 - easeOutBounce(1 - 2 * x)) / 2 : (1 + easeOutBounce(2 * x - 1)) / 2;
}

// Elastic Easing
float easeInElastic(float x) {
    const float c4 = (2 * M_PI) / 3;

    return x == 0 ? 0 : x == 1 ? 1 : -powf(2, 10 * x - 10) * sinf((x * 10 - 10.75f) * c4);
}

float easeOutElastic(float x) {
    const float c4 = (2 * M_PI) / 3;

    return x == 0 ? 0 : x == 1 ? 1 : powf(2, -10 * x) * sinf((x * 10 - 0.75f) * c4) + 1;
}

float easeInOutElastic(float x) {
    const float c5 = (2 * M_PI) / 4.5;

    if (x == 0) return 0;
    if (x == 1) return 1;
    return x < 0.5 ? -(powf(2, 20 * x - 10) * sinf((20 * x - 11.125f) * c5)) / 2
                   : (powf(2, -20 * x + 10) * sinf((20 * x - 11.125f) * c5)) / 2 + 1;
}

// Back Easing
float easeInBack(float x) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1;

    return c3 * x * x * x - c1 * x * x;
}

float easeOutBack(float x) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1;

    return 1 + c3 * powf(x - 1, 3) + c1 * powf(x - 1, 2);
}

float easeInOutBack(float x) {
    const float c1 = 1.70158f;
    const float c2 = c1 * 1.525f;

    return x < 0.5 ? (powf(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2
                   : (powf(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
}

#endif // EASING_IMPLEMENTATION
