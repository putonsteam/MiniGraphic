#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"

#include <memory>
#include <vector>

using std::shared_ptr;
using std::make_shared;

class hittableList : public hittable {
public:
    hittableList() {}
    hittableList(shared_ptr<hittable> object) 
    { 
        Add(object); 
    }

    void clear() { mObjects.clear(); }
    void Add(shared_ptr<hittable> inObject) 
    { 
        mObjects.push_back(inObject); 
    }

    virtual bool Hit(const ray&, double, double, HitRecord&) const override;

public:
    std::vector<shared_ptr<hittable>> mObjects;
};

bool hittableList::Hit(const ray& inRay, double inTMin, double inTMax, HitRecord& inRecord) const
{
    HitRecord temp_rec;
    bool hit_anything = false;
    double closest_so_far = inTMax;

    for (const auto& object : mObjects) {
        if (object->Hit(inRay, inTMin, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            inRecord = temp_rec;
        }
    }

    return hit_anything;
}

#endif
