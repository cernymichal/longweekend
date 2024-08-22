#pragma once

#include "IHittable.h"
#include "Transform.h"

class Rectangle : public IHittable {
public:
    Transform m_transform;
    Ref<Material> m_material;

    Rectangle(const Transform& transform, const Ref<Material>& material) : m_transform(transform), m_material(material) {}

    HitRecord hit(Ray& ray) const override {
        Ray transformedRay = ray.createTransformedRay(m_transform.modelMatrixInverse());

        if (std::abs(transformedRay.direction.y) < 1e-6f)
            return HitRecord();

        f32 t = -transformedRay.origin.y / transformedRay.direction.y;
        vec3 hitPoint = transformedRay.at(t);

        HitRecord hit;
        hit.hit = transformedRay.tInterval.surrounds(t) && std::abs(hitPoint.x) <= 0.5f && std::abs(hitPoint.z) <= 0.5f;

        if (hit.hit) {
            transformedRay.tInterval.max = t;
            hit.normal = transformedRay.direction.y < 0.0f ? vec3(0.0f, 1.0f, 0.0f) : vec3(0.0f, -1.0f, 0.0f);  // TODO only front face based on material
            hit.material = m_material;

            ray.updateFromTransformedRay(transformedRay, m_transform.modelMatrix());
            hit.transform(m_transform.modelMatrix());
        }

        return hit;
    }

    void frameBegin() override {
        m_transform.updateMatrices();
    }
};
