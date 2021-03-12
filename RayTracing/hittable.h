#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"

class material;

struct HitRecord {
    vec3 p;
    vec3 normal;
    double t;
    shared_ptr<material> material_ptr;

    bool front_face;

    inline void SetFaceNormal(const ray& inRay, const vec3& inOutwardNormal) {
        front_face = dot(inRay.GetDirection(), inOutwardNormal) < 0;
        normal = front_face ? inOutwardNormal : -inOutwardNormal;
    }
};

class hittable {
public:
    virtual bool Hit(const ray& r, double t_min, double t_max, HitRecord& rec) const = 0;
};

#endif