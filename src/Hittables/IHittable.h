#pragma once

#include "Ray.h"

class Material;

struct HitRecord {
    bool hit = false;
    f32 t = 0.0f;
    vec3 point = vec3(0);
    vec3 normal = vec3(0);
    bool frontFace = false;
    vec2 uv = vec2(0);
    Ref<Material> material;

    void setNormal(const Ray& ray, const vec3& outwardNormal) {
        // outwardNormal is assumed to be normalized

        frontFace = glm::dot(ray.direction, outwardNormal) <= 0;
        normal = frontFace ? outwardNormal : -outwardNormal;
    }
};

class IHittable {
public:
    virtual HitRecord hit(const Ray& ray, Interval<f32> tInterval) const = 0;
};
