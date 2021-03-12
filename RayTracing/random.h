#ifndef RAMDOM_H
#define RAMDOM_H

#include <cstdlib> //for rand()

inline double gRandomDouble()
{
    // Returns a random real in [0,1).
    return rand() / (RAND_MAX + 1.0);
}

inline double gRandomDouble(double min, double max)
{
    // Returns a random real in [min,max).
    return min + (max - min) * gRandomDouble();
}

#endif