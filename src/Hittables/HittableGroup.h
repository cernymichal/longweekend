#pragma once

#include "IHittable.h"

class HittableGroup : public IHittable {
public:
    HittableGroup() = default;

    HittableGroup(std::vector<Ref<IHittable>> hittables) : m_hittables(hittables) {}

    void add(Ref<IHittable> hittable) {
        m_hittables.push_back(hittable);
    }

    void clear() {
        m_hittables.clear();
    }

    HitRecord hit(const Ray& ray, Interval<f32> tInterval) const override {
        HitRecord hit{false};

        for (const auto& hittable : m_hittables) {
            auto childHit = hittable->hit(ray, tInterval);

            if (childHit.hit) {
                hit = childHit;
                tInterval.max = hit.t;
            }
        }

        return hit;
    }

private:
    std::vector<Ref<IHittable>> m_hittables;
};
