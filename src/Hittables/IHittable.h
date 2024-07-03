#pragma once

#include "Face.h"
#include "Ray.h"

class Material;

struct HitRecord {
    // Set by the hit function
    bool hit = false;
    vec3 normal = vec3(0);
    vec3 barycentric = vec3(0);  // TODO only two
    const Face* face = nullptr;
    WeakRef<const Material> material;
    vec2 uv = vec2(0);  // TODO remove

    vec3 point;  // Set just before scattering

    void transform(const mat4& transform) {
        normal = glm::normalize(vec3(transform * vec4(normal, 0)));
    }
};

class IHittable {
public:
    virtual HitRecord hit(Ray& ray) const = 0;

    virtual void frameBegin() {}
};
