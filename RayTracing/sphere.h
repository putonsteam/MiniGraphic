#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "vec3.h"

class sphere : public hittable {
public:
    sphere() {}
    sphere(vec3 inCenter, double inRadius) : 
        mCenter(inCenter), 
        mRadius(inRadius)
    {};

	sphere(vec3 inCenter, double inRadius, shared_ptr<material> inMaterialPtr) :
		mCenter(inCenter),
		mRadius(inRadius),
        mMaterialPtr(inMaterialPtr)
	{};

    virtual bool Hit(const ray&, double, double, HitRecord&) const override;

public:
    vec3 mCenter;
    double mRadius;
    shared_ptr<material> mMaterialPtr;
};



bool sphere::Hit(const ray& inRay, double inTMin, double inTMax, HitRecord& inRecord) const 
{
    vec3 oc = inRay.GetOrigin() - mCenter;
    auto a = inRay.GetDirection().length_squared();
    auto half_b = dot(oc, inRay.GetDirection());
    auto c = oc.length_squared() - mRadius * mRadius;

    double discriminant = half_b * half_b - a * c;
    if (discriminant < 0) 
        return false;
    double sqrtd = sqrt(discriminant);

    // Find the nearest root that lies in the acceptable range.
    double root = (-half_b - sqrtd) / a;
    if (root < inTMin || root > inTMax) {
        //why do we need a further point?
        //note: check another point(eg. if the camera is inside a huge sphere and the closest point is at the back of the camera)
        root = (-half_b + sqrtd) / a;
        if (root < inTMin || root > inTMax)//we need to do another check
            return false;
    }

    inRecord.t = root;
    inRecord.p = inRay.At(inRecord.t);
    vec3 outward_normal = (inRecord.p - mCenter) / mRadius;
    inRecord.SetFaceNormal(inRay, outward_normal);
    inRecord.material_ptr = mMaterialPtr;

    return true;

}

#endif
