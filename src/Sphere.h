#pragma once

#include "IHittable.h"

class Sphere : public IHittable {
public:
    vec3 m_center;
    float m_radius;
    Ref<Material> m_material;

    Sphere(const vec3& center, float radius, const Ref<Material>& material) : m_center(center), m_radius(radius), m_material(material) {}

    HitRecord hit(const Ray& ray, Interval<float> tInterval) const override {
        auto a = glm::length2(ray.direction());
        auto halfB = dot(ray.direction(), ray.origin() - m_center);
        auto c = dot(ray.origin() - m_center, ray.origin() - m_center) - m_radius * m_radius;
        auto discriminant = halfB * halfB - a * c;

        if (discriminant < 0)
            return HitRecord();

        auto t = (-halfB - sqrt(discriminant)) / a;

        if (!tInterval.surrounds(t)) {
            t = (-halfB + sqrt(discriminant)) / a;
            if (!tInterval.surrounds(t))
                return HitRecord();
        }

        HitRecord hit{true};
        hit.t = t;
        hit.point = ray.at(t);
        auto outwardNormal = (ray.at(t) - m_center) / m_radius;
        hit.setFaceNormal(ray, outwardNormal);
        hit.material = m_material;

        return hit;
    }
};
