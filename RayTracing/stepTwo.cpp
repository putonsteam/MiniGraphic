#include <iostream>
#include <fstream>

////NEW!!!!///////
#include "color.h"
#include "vec3.h"
//////////////////
using namespace std;

int main() {

    ofstream out_stream;
    out_stream.open("helloWorld.ppm");

    // Image
    const int image_width = 200;
    const int image_height = 100;

    // Render
    out_stream << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = image_height - 1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            ////NEW!!!!///////
            color pixel_color(double(i) / (image_width - 1), double(j) / (image_height - 1), 0.25);
            WriteColor(out_stream, pixel_color);
            //////////////////
        }
    }

    out_stream.close();
}

