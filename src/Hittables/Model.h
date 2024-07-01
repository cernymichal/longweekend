#pragma once

#include "BVH/BVH.h"
#include "Face.h"
#include "IHittable.h"
#include "Transform.h"

class Mesh : public IHittable {
public:
    struct Submesh {
        std::vector<Face> faces;  // TODO index buffer?
        BVH bvh;
        Ref<Material> material;
    };

    std::vector<Submesh> m_submeshes;
    bool m_hasNormals;
    bool m_hasUVs;

    Mesh(std::vector<Submesh>&& submeshes) : m_submeshes(submeshes) {}

    HitRecord hit(const Ray& ray, Interval<f32> tInterval) const override {
        HitRecord hit{false};
        for (const auto& submesh : m_submeshes) {
            HitRecord submeshHit = submesh.bvh.hit(ray, tInterval);

            // glm::normalize(glm::cross(face.vertices[1] - face.vertices[0], face.vertices[2] - face.vertices[0]))

            if (submeshHit.hit) {
                hit = submeshHit;
                hit.material = submesh.material;
                tInterval.max = hit.t;
            }
        }

        return hit;
    }
};

class Model : public IHittable {
public:
    Transform m_transform;
    Ref<Mesh> m_mesh;

    Model(const Ref<Mesh>& mesh, const Transform& transform = Transform()) : m_mesh(mesh), m_transform(transform) {}

    HitRecord hit(const Ray& ray, Interval<f32> tInterval) const override {
        auto transformedRay = m_transform.modelMatrixInverse() * ray;

        // TODO verify this
        vec3 tMinVec = m_transform.modelMatrixInverse() * vec4(tInterval.min * ray.direction, 0);
        vec3 tMaxVec = m_transform.modelMatrixInverse() * vec4(tInterval.max * ray.direction, 0);
        auto transformedTInterval = Interval<f32>(glm::length(tMinVec), glm::length(tMaxVec));

        HitRecord hit = m_mesh->hit(transformedRay, transformedTInterval);

        if (hit.hit) {
            hit.point = m_transform.modelMatrix() * vec4(hit.point, 1);
            hit.t = glm::length(hit.point - ray.origin) / glm::length(ray.direction);
            hit.setNormal(ray, glm::normalize(vec3(m_transform.modelMatrix() * vec4(hit.normal, 0))));
        }

        return hit;
    }
};
