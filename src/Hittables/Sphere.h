#pragma once

#include "IHittable.h"

class Sphere : public IHittable {
public:
    vec3 m_center;
    f32 m_radius;
    Ref<Material> m_material;

    Sphere(const vec3& center, f32 radius, const Ref<Material>& material) : m_center(center), m_radius(radius), m_material(material) {}

    HitRecord hit(Ray& ray) const override {
        f32 halfB = glm::dot(ray.direction, ray.origin - m_center);
        f32 c = glm::dot(ray.origin - m_center, ray.origin - m_center) - m_radius * m_radius;
        f32 discriminant = halfB * halfB - c;

        if (discriminant < 0)
            return HitRecord();

        f32 t = -halfB - sqrt(discriminant);

        // TODO only front face intersection based on material
        if (!ray.tInterval.surrounds(t)) {
            t = -halfB + sqrt(discriminant);
            if (!ray.tInterval.surrounds(t))
                return HitRecord();
        }

        HitRecord hit;
        hit.hit = true;
        ray.tInterval.max = t;
        hit.normal = (ray.at(t) - m_center) / m_radius;
        hit.material = m_material;

        return hit;
    }
};
