#include <iostream>
#include <fstream>

#include "color.h"
#include "common.h"
#include "hittableList.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"

using namespace std;


color RayColor(const ray& inRay, const hittable& inHittable, int inMaxDepth) {
	// If we've exceeded the ray bounce limit, no more light is gathered.
	if (inMaxDepth <= 0)
		return color(0, 0, 0);

	HitRecord record;

	if (inHittable.Hit(inRay, 0.001, cInfinity, record))
	{
		ray scattered;
		color attenuation;

		if (record.material_ptr->scatter(inRay, record, attenuation, scattered))
			return attenuation * RayColor(scattered, inHittable, inMaxDepth - 1);

		return color(0, 0, 0);
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
	const int image_width = 1000;
	const int image_height = static_cast<int>(image_width / aspect_ratio);
	const int samples_per_pixel = 200;
	const int max_depth = 40;


	// Hittables
	hittableList hittables_list;
	auto material_ground = make_shared<lambertian>(color(0.88, 0.96, 1.0));
	////NEW!!!!///////
	auto material_center = make_shared<dielectric>(3.1);
	//////////////////
	auto material_left = make_shared<lambertian>(color(0.08, 0.36, 0.6));
	auto material_right = make_shared<lambertian>(color(1.0, 0.94, 0.58));

	hittables_list.Add(make_shared<sphere>(vec3(0.0, -100.5, -1.0), 100.0, material_ground));
	hittables_list.Add(make_shared<sphere>(vec3(0.0, 0.0, -1.0), 0.5, material_center));
	hittables_list.Add(make_shared<sphere>(vec3(-0.5, 0.0, -2.5), 0.5, material_left));
	hittables_list.Add(make_shared<sphere>(vec3(0.5, 0.0, -2.5), 0.5, material_right));


	// Camera
	//camera cam;
	camera cam(vec3(-2, 2, 1), vec3(0, 0, -1), vec3(0, 1, 0), 90, aspect_ratio);



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

	out_stream.close();
}

