#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class ray {
public:
    ray() {}

    ray(const vec3& inOrigin, const vec3& inDirection): 
    mOrigin(inOrigin), 
    mDirection(inDirection)
    {}

    vec3 GetOrigin() const { return mOrigin; }
    vec3 GetDirection() const { return mDirection; }

    vec3 At(double t) const 
    {
        return mOrigin + t * mDirection;
    }

public:
    vec3 mOrigin;
    vec3 mDirection;
};
#endif
