#pragma once

#include "HitRecord.h"
#include "Ray.h"

class IHittable {
public:
    virtual HitRecord hit(Ray& ray) const = 0;

    virtual void frameBegin() {}
};
