#include "gfx/color.h"

namespace arc {

color color::from_bytes(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return color(r / 255.0, g / 255.0, b / 255.0, a / 255.0);
}

void color::get_bytes(uint8_t* br, uint8_t* bg, uint8_t* bb, uint8_t* ba) {
    *br = (uint8_t)(255 * r);
    *bg = (uint8_t)(255 * g);
    *bb = (uint8_t)(255 * b);
    *ba = (uint8_t)(255 * a);
}

color color::operator+(double s) const { return color(r + s, g + s, b + s, a); }
color color::operator-(double s) const { return color(r - s, g - s, b - s, a); }
color color::operator/(double s) const { return color(r / s, g / s, b / s, a); }

color color::hsv(float hue, float saturation, float value) {
    int i = static_cast<int>(hue * 6) % 6;
    float f = hue * 6 - i;
    float f1 = value * (1 - saturation);
    float f2 = value * (1 - f * saturation);
    float f3 = value * (1 - (1 - f) * saturation);
    float f4 = 0;
    float f5 = 0;
    float f6 = 0;

    switch (i) {
        case 0:
            f4 = value;
            f5 = f3;
            f6 = f1;
            break;
        case 1:
            f4 = f2;
            f5 = value;
            f6 = f1;
            break;
        case 2:
            f4 = f1;
            f5 = value;
            f6 = f3;
            break;
        case 3:
            f4 = f1;
            f5 = f2;
            f6 = value;
            break;
        case 4:
            f4 = f3;
            f5 = f1;
            f6 = value;
            break;
        case 5:
            f4 = value;
            f5 = f1;
            f6 = f2;
            break;
    }

    return color(f4, f5, f6);
}

}  // namespace arc