#pragma once

#include "Ray.h"

class Material;

struct HitRecord {
    bool hit = false;
    f32 t = 0.0f;
    vec3 point = vec3(0);
    vec3 normal = vec3(0);
    bool frontFace = false;
    Ref<Material> material;

    void setFaceNormal(const Ray& ray, const vec3& outwardNormal) {
        // outwardNormal is assumed to be normalized

        frontFace = dot(ray.direction(), outwardNormal) <= 0;
        normal = frontFace ? outwardNormal : -outwardNormal;
    }
};

class IHittable {
public:
    virtual HitRecord hit(const Ray& ray, Interval<f32> tInterval) const = 0;
};
