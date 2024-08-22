#pragma once

#include "IHittable.h"

class InfinitePlane : public IHittable {
public:
    vec3 m_origin;
    vec3 m_normal;
    Ref<Material> m_material;

    InfinitePlane(const vec3& origin, const vec3& normal, const Ref<Material>& material) : m_origin(origin), m_normal(normal), m_material(material) {}

    HitRecord hit(Ray& ray) const override {
        float dot = glm::dot(m_normal, ray.direction);
        if (std::abs(dot) < 1e-6f)  // TODO front face based on material
            return HitRecord();

        f32 t = glm::dot(m_origin - ray.origin, m_normal) / dot;

        if (!ray.tInterval.surrounds(t))
            return HitRecord();

        HitRecord hit;
        hit.hit = true;
        ray.tInterval.max = t;
        hit.normal = dot < 0.0f ? m_normal : -m_normal;
        hit.material = m_material;

        return hit;
    }
};
