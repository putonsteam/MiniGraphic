﻿#include <iostream>
#include <fstream>

#include "color.h"
#include "common.h"
#include "hittableList.h"
#include "sphere.h"
#include "camera.h"

using namespace std;





////NEW!!!!///////
const float attenuation = 0.5f;
////NEW!!!!///////

ofstream debug_text;
int return_black = 0;
int return_sky = 0;

color RayColor(const ray& inRay, const hittable& inHittable, int inMaxDepth) {
    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (inMaxDepth <= 0)
    {
        debug_text << "return (0,0,0)"  << "\n";
        return_black++;
        return color(0, 0, 0);
    }
       

    HitRecord record;
    //if (inHittable.Hit(inRay, 0, cInfinity, record))
    if (inHittable.Hit(inRay, 0.001, cInfinity, record))
    {
        //Pick a random point S inside this unit radius sphere and 
        //send a ray from the hit point P to the random point S (this is the vector (S−P))
        vec3 target = record.p + record.normal + vec3::GetRandomUnitVector();

        ////NEW!!!!///////
        return attenuation * RayColor(ray(record.p, target - record.p), inHittable, inMaxDepth - 1);
        //////////////////
    }

    vec3 unit_direction = unit_vector(inRay.GetDirection()); // -1 < unit_direction < 1
    double t = 0.5 * (unit_direction.y() + 1.0); // 0 <= t <= 1.0

    debug_text << "inMaxDepth = " << inMaxDepth << "\n";
    return_sky++;
    return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0); //linearly blend white and blue
}



int main() {

    ofstream out_stream;
    out_stream.open("helloWorld.ppm");
    debug_text.open("log.txt");


    // Image
    const float aspect_ratio = 16.0f / 9.0f;
    const int image_width = 400;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 64;
    const int max_depth = 10;


    // Hittables
    hittableList hittables_list;
    hittables_list.Add(make_shared<sphere>(vec3(0, 0, -1), 0.5));
    hittables_list.Add(make_shared<sphere>(vec3(0, -100.5, -1), 100));


    // Camera
    camera cam;



    // Render
    out_stream << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = image_height - 1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            color pixel_color(0, 0, 0);

            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + gRandomDouble()) / (image_width - 1);
                auto v = (j + gRandomDouble()) / (image_height - 1);
                ray new_ray = cam.GetRay(u, v);
                pixel_color += RayColor(new_ray, hittables_list, max_depth);
            }

            WriteColorMultipleSamples(out_stream, pixel_color, samples_per_pixel);
        }
    }

    debug_text << "return black = " << return_black << ",return sky = " << return_sky << "\n";

    out_stream.close();
    debug_text.close();
}

