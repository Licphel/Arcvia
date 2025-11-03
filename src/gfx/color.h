#pragma once
#include <cstdint>

namespace arc {

struct color {
    double r = 1.0;
    double g = 1.0;
    double b = 1.0;
    double a = 1.0;

    // this operation is used so frequently so that we inline it.
    inline color() = default;
    // this operation is used so frequently so that we inline it.
    inline color(double x, double y, double z) {
        r = x;
        g = y;
        b = z;
        a = 1.0;
    }
    // this operation is used so frequently so that we inline it.
    inline color(double x, double y, double z, double w) {
        r = x;
        g = y;
        b = z;
        a = w;
    }

    static color from_bytes(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void get_bytes(uint8_t* br, uint8_t* bg, uint8_t* bb, uint8_t* ba);

    // this operation is used so frequently so that we inline it.
    inline color operator*(const color& v) const { return color(r * v.r, g * v.g, b * v.b, a * v.a); }

    // color operation, with a number, does not touch alpha.
    color operator+(double s) const;
    color operator-(double s) const;

    // this operation is used so frequently so that we inline it.
    color operator*(double s) const { return color(r * s, g * s, b * s, a); }
    color operator/(double s) const;

    static color hsv(float hue, float saturation, float value);
};

}  // namespace arc