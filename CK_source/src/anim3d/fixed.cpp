// fixed.cpp - 20.12 fixed point square root and sine (created by b047x).
#include "Fixed.h"

const float Fixed::FIXED_TO_FLOAT_COEF = 1.0f / 4096.0f;

const int Fixed::SIN_COS_TABLE[91] = {
    0, 71, 142, 214, 285, 356, 428, 499,
    570, 640, 711, 781, 851, 921, 990, 1060,
    1129, 1197, 1265, 1333, 1400, 1467, 1534, 1600,
    1665, 1731, 1795, 1859, 1922, 1985, 2048, 2109,
    2170, 2230, 2290, 2349, 2407, 2465, 2521, 2577,
    2632, 2687, 2740, 2793, 2845, 2896, 2946, 2995,
    3043, 3091, 3137, 3183, 3227, 3271, 3313, 3355,
    3395, 3435, 3473, 3510, 3547, 3582, 3616, 3649,
    3681, 3712, 3741, 3770, 3797, 3823, 3848, 3872,
    3895, 3917, 3937, 3956, 3974, 3991, 4006, 4020,
    4033, 4045, 4056, 4065, 4073, 4080, 4086, 4090,
    4093, 4095, 4096,
};

/** Sine of a whole degree (any non-negative angle) from the quarter table. */
static inline int sinOfDegree(int degree)
{
    int index;
    if (degree % 180 < 90) {
        index = degree % 90;
    } else {
        index = 90 - degree % 90;
    }
    int sign;
    if (degree % 360 < 180) {
        sign = 1;
    } else {
        sign = -1;
    }
    return Fixed::SIN_COS_TABLE[index] * sign;
}

int Fixed::sqrt(int value)
{
    int root = (value + 0x1000) >> 1;
    for (int step = 0; step < 8; step++) {
        root = (div(value, root) + root) >> 1;
    }
    return root;
}

int Fixed::sin(int angle)
{
    // bring the angle into [0, 360) degrees plus one turn, so it is positive
    angle = angle % (360 << 12) + (360 << 12);
    int degree = angle >> 12;
    int value = sinOfDegree(degree);
    int nextDegree = (angle + 0x1000) >> 12;
    if (degree == nextDegree) {
        return value;
    }
    int nextValue = sinOfDegree(nextDegree);
    return value + (((nextValue - value) * (angle & 0xfff)) >> 12);
}

int Fixed::rad2grad(int radians)
{
    // 0x394BB = 57.2958 (180 / pi) in 20.12
    return mul(radians, 0x394BB);
}

/** Square in 20.12, combining the two equal cross products. */
static inline int squareFixed(int value)
{
    int fraction = value & 0xfff;
    int integer = value >> 12;
    return integer * ((value & ~0xfff) + (fraction << 1)) + ((fraction * fraction) >> 12);
}

int Fixed::atan(int value)
{
    // Euler: atan(x) = x / (1 + x^2) * sum(c_n * y^n), y = x^2 / (1 + x^2)
    int square = squareFixed(value);
    int denominator = square + 0x1000;
    int y = div(square, denominator);
    int y2 = squareFixed(y);
    int y3 = mul(y2, y);
    int y4 = squareFixed(y2);
    int y5 = mul(y2, y3);
    float series = (float)y * (2.0f / 3.0f);
    series = series + (float)y2 * (8.0f / 15.0f);
    series = series + (float)y3 * (16.0f / 35.0f);
    series = series + (float)y4 * (128.0f / 315.0f);
    series = series + (float)y5 * (256.0f / 693.0f);
    int sum = (int)series + 0x1000;
    return (int)((float)value / (float)denominator * (float)sum);
}
