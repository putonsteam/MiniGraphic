#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"

class camera {
public:
    camera() {
        float aspect_ratio = 16.0f / 9.0f;
        float viewport_height = 2.0f;
        float viewport_width = aspect_ratio * viewport_height;
        float focal_length = 1.0f;

        mOrigin = vec3(0, 0, 0);
        mHorizontal = vec3(viewport_width, 0.0, 0.0);
        mVertical = vec3(0.0, viewport_height, 0.0);
        mLowerLeftCorner = mOrigin - mHorizontal / 2 - mVertical / 2 - vec3(0, 0, focal_length);
    }

	camera(
        vec3 lookfrom,
        vec3 lookat,
		vec3 vup,
		double vfov, // vertical field-of-view in degrees
		double aspect_ratio
	) {
		auto theta = gDegreesToRadians(vfov);
		auto h = tan(theta / 2);
		auto viewport_height = 2.0 * h;
		auto viewport_width = aspect_ratio * viewport_height;

		auto w = unit_vector(lookfrom - lookat);
		auto u = unit_vector(cross(vup, w));
		auto v = cross(w, u);

		mOrigin = lookfrom;
		mHorizontal = viewport_width * u;
		mVertical = viewport_height * v;
		mLowerLeftCorner = mOrigin - mHorizontal / 2 - mVertical / 2 - w;
	}


    ray GetRay(double u, double v) const 
    {
        return ray(mOrigin, mLowerLeftCorner + u * mHorizontal + v * mVertical - mOrigin);
    }

private:
    vec3 mOrigin;
    vec3 mLowerLeftCorner;
    vec3 mHorizontal;
    vec3 mVertical;
};
#endif
