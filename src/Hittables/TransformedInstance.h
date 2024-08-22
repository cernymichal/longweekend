#pragma once

#include "IHittable.h"
#include "Transform.h"

template <typename T>
class TransformedInstance : public IHittable {
public:
    Transform m_transform;
    Ref<T> m_hittable;

    TransformedInstance(const Ref<T>& hittable, const Transform& transform = Transform()) : m_hittable(hittable), m_transform(transform) {}

    HitRecord hit(Ray& ray) const override {
        static_assert(std::is_base_of_v<IHittable, T>, "T must derive from IHittable");

        Ray transformedRay = ray.createTransformedRay(m_transform.modelMatrixInverse());
        HitRecord hit = m_hittable->hit(transformedRay);

        if (hit.hit) {
            ray.updateFromTransformedRay(transformedRay, m_transform.modelMatrix());
            hit.transform(m_transform.modelMatrix());
        }

        return hit;
    }

    void frameBegin() override {
        m_transform.updateMatrices();
        m_hittable->frameBegin();
    }
};
