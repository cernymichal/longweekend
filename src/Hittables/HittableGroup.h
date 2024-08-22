#pragma once

#include "IHittable.h"

class HittableGroup : public IHittable {
public:
    HittableGroup() = default;

    explicit HittableGroup(std::vector<Ref<IHittable>> hittables) : m_hittables(hittables) {}

    void add(Ref<IHittable> hittable) {
        m_hittables.push_back(hittable);
    }

    void clear() {
        m_hittables.clear();
    }

    HitRecord hit(Ray& ray) const override {
        HitRecord hit;

        for (const auto& hittable : m_hittables) {
            HitRecord childHit = hittable->hit(ray);
            if (childHit.hit)
                hit = childHit;
        }

        return hit;
    }

    void frameBegin() override {
        for (size_t i = 0; i < m_hittables.size(); i++)
            m_hittables[i]->frameBegin();
    }

private:
    std::vector<Ref<IHittable>> m_hittables;
};
