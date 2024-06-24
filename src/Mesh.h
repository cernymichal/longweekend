#pragma once

#include "IHittable.h"

class Mesh : public IHittable {
public:
    struct Face {
        std::array<vec3, 3> vertices;
        std::array<vec2, 3> uvs;
        vec3 normal;
    };

    std::vector<Face> m_faces;
    Ref<Material> m_material;

    // TODO transform

    Mesh(std::vector<Face>&& faces, const Ref<Material>& material) : m_faces(faces), m_material(material) {}

    HitRecord hit(const Ray& ray, Interval<f32> tInterval) const override {
        for (const auto& face : m_faces) {
            auto t = rayTriangleIntersection(ray.origin(), ray.direction(), face.vertices);

            if (isnan(t) || !tInterval.surrounds(t))
                continue;

            HitRecord hit{true};
            hit.t = t;
            hit.point = ray.at(t);
            hit.setFaceNormal(ray, face.normal);
            hit.material = m_material;

            return hit;
        }

        return HitRecord();
    }
};
