#pragma once

#include "Hittables/IHittable.h"

class BVH {
public:
	HitRecord hit(const Ray& ray, Interval<f32> tInterval) const;
};

