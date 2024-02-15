#pragma once

#include "Ray.h"

struct HitRecord {
    bool hit = false;
    float t = 0.0f;
    vec3 point = vec3(0);
    vec3 normal = vec3(0);
    bool frontFace = false;

    void setFaceNormal(const Ray& ray, const vec3& outwardNormal) {
        // outwardNormal is assumed to be normalized

        frontFace = dot(ray.direction(), outwardNormal) <= 0;
        normal = frontFace ? outwardNormal : -outwardNormal;
    }
};

class IHittable {
public:
    virtual HitRecord hit(const Ray& ray, Interval<float> tInterval) const = 0;
};
