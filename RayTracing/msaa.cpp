#include <iostream>
#include <fstream>

#include "color.h"
#include "common.h"
#include "hittableList.h"
#include "sphere.h"

////NEW!!!!///////
#include "camera.h"
//////////////////
using namespace std;


color RayColor(const ray& inRay, const hittable& inHittable) {

    HitRecord record;
    if (inHittable.Hit(inRay, 0, cInfinity, record))
    {
        return 0.5 * (record.normal + color(1, 1, 1));//convert from (-1,1) to [0,1] 
    }

    vec3 unit_direction = unit_vector(inRay.GetDirection()); // -1 < unit_direction < 1
    double t = 0.5 * (unit_direction.y() + 1.0); // 0 <= t <= 1.0
    return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0); //linearly blend white and blue
}



int main() {

    ofstream out_stream;
    out_stream.open("helloWorld.ppm");

    // Image
    const float aspect_ratio = 16.0f / 9.0f;
    const int image_width = 400;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    ////NEW!!!!///////
    const int samples_per_pixel = 100;
    //////////////////


    // Hittables
    hittableList hittables_list;
    hittables_list.Add(make_shared<sphere>(vec3(0, 0, -1), 0.5));
    hittables_list.Add(make_shared<sphere>(vec3(0, -100.5, -1), 100));
    

    ////NEW!!!!///////
    // Camera
    camera cam;
    //////////////////


    // Render
    out_stream << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = image_height - 1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            ////NEW!!!!///////
            color pixel_color(0, 0, 0);

            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + gRandomDouble()) / (image_width - 1);
                auto v = (j + gRandomDouble()) / (image_height - 1);
                ray new_ray = cam.GetRay(u, v);
                pixel_color += RayColor(new_ray, hittables_list);
            }
           
            WriteColorMultipleSamples(out_stream, pixel_color, samples_per_pixel);
            //////////////////
        }
    }

    out_stream.close();
}

