#include <iostream>
#include <fstream>

#include "color.h"
#include "vec3.h"

////NEW!!!!///////
#include "ray.h"
//////////////////
using namespace std;



////NEW!!!!///////
color RayColor(const ray& inRay) {
    vec3 unit_direction = unit_vector(inRay.GetDirection()); // -1 < unit_direction < 1
    double t = 0.5 * (unit_direction.y() + 1.0); // 0 <= t <= 1.0
    return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0); //linearly blend white and blue
}
//////////////////


int main() {

    ofstream out_stream;
    out_stream.open("helloWorld.ppm");


    ////NEW!!!!///////

    // Image
    const float aspect_ratio = 16.0f / 9.0f;
    const int image_width = 400;
    const int image_height = static_cast<int>(image_width / aspect_ratio);


    // Camera
    float viewport_height = 2.0f;
    float viewport_width = aspect_ratio * viewport_height;
    float focal_length = 1.0f;

    vec3 origin = vec3(0, 0, 0);
    vec3 horizontal = vec3(viewport_width, 0, 0);
    vec3 vertical = vec3(0, viewport_height, 0);
    vec3 lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);

    //////////////////



    // Render
    out_stream << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = image_height - 1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            ////NEW!!!!///////
            double u = double(i) / (image_width - 1);
            double v = double(j) / (image_height - 1);
            ray new_ray(origin, lower_left_corner + u * horizontal + v * vertical - origin);

            color pixel_color = RayColor(new_ray);
            //////////////////

            WriteColor(out_stream, pixel_color);
        }
    }

    out_stream.close();
}

