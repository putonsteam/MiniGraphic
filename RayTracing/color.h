#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"

#include <iostream>



double clamp(double x, double min, double max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}


void WriteColor(std::ostream& out, color pixel_color) {
    // Write the translated [0,255] value of each color component.
    out << static_cast<int>(255.999 * pixel_color.x()) << ' '
        << static_cast<int>(255.999 * pixel_color.y()) << ' '
        << static_cast<int>(255.999 * pixel_color.z()) << '\n';
}



/*Rather than adding in a fractional contribution each time we accumulate more light to the color, 
just add the full color each iteration, and then perform a single divide 
at the end(by the number of samples) when writing out the color.*/
void WriteColorMultipleSamples(std::ostream& out, color inPixelColor, int inSamplesPerPixel) {
    auto r = inPixelColor.x();
    auto g = inPixelColor.y();
    auto b = inPixelColor.z();

    // Divide the color by the number of samples.
    auto scale = 1.0 / inSamplesPerPixel;
    r *= scale;
    g *= scale;
    b *= scale;

    // Write the translated [0,255] value of each color component.
    out << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';
}
#endif
