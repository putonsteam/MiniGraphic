#include <iostream>
#include <fstream>

#include "color.h"
#include "common.h"
#include "hittableList.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"
using namespace std;



hittableList RandomScene()
{
	hittableList world;

	auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
	world.Add(make_shared<sphere>(vec3(0, -1000, 0), 1000, ground_material));

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			auto choose_mat = gRandomDouble();
			vec3 center(a + 0.9 * gRandomDouble(), 0.2, b + 0.9 * gRandomDouble());

			if ((center - vec3(4, 0.2, 0)).length() > 0.9) {
				shared_ptr<material> sphere_material;

				if (choose_mat < 0.8) {
					// diffuse
					auto albedo = color::random() * color::random();
					sphere_material = make_shared<lambertian>(albedo);
					world.Add(make_shared<sphere>(center, 0.2, sphere_material));
				}
				else if (choose_mat < 0.95) {
					// metal
					auto albedo = color::random(0.5, 1);
					auto fuzz = gRandomDouble(0, 0.5);
					sphere_material = make_shared<metal>(albedo, fuzz);
					world.Add(make_shared<sphere>(center, 0.2, sphere_material));
				}
				else {
					// glass
					sphere_material = make_shared<dielectric>(1.5);
					world.Add(make_shared<sphere>(center, 0.2, sphere_material));
				}
			}
		}
	}

	auto material1 = make_shared<dielectric>(1.5);
	world.Add(make_shared<sphere>(vec3(0, 1, 0), 1.0, material1));

	auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
	world.Add(make_shared<sphere>(vec3(-4, 1, 0), 1.0, material2));

	auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
	world.Add(make_shared<sphere>(vec3(4, 1, 0), 1.0, material3));

	return world;
}



color RayColor(const ray& inRay, const hittable& inHittable, int inMaxDepth) 
{
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
	////NEW!!!!///////
	const float aspect_ratio = 3.0f / 2.0f;
	const int image_width = 1000;
	//////////////////
	const int image_height = static_cast<int>(image_width / aspect_ratio);
	const int samples_per_pixel = 100;
	const int max_depth = 30;


	// Hittables
	////NEW!!!!///////
	hittableList hittables_list = RandomScene();
	//////////////////

	


	// Camera
	vec3 lookfrom(13, 2, 3);
	vec3 lookat(0, 0, 0);
	vec3 vup(0, 1, 0);
	camera cam(lookfrom, lookat, vup,  20, aspect_ratio);



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

