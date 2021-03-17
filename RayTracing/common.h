#ifndef COMMON_H
#define COMMON_H

#include <cmath>
#include <limits>
#include <memory> //for shared_ptr

// Common Headers
#include "ray.h"
#include "vec3.h"

// Usings
using std::shared_ptr;
using std::make_shared;
using std::sqrt;


// Constants
const double cInfinity = std::numeric_limits<double>::infinity();
const double cPi = 3.1415926535897932385;


// Utility Functions
inline double gDegreesToRadians(double degrees) 
{
    return degrees * cPi / 180.0;
}



#endif