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
        HitRecord closestHit;
        closestHit.t = tInterval.max;

        for (const auto& hittable : m_hittables) {
            auto hit = hittable->hit(ray, {tInterval.min, closestHit.t});

            if (hit.hit)
                closestHit = hit;
        }

        return closestHit;
    }

private:
    std::vector<Ref<IHittable>> m_hittables;
};
