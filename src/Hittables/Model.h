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

    std::string m_name = "Unnamed";
    std::vector<Submesh> m_submeshes;
    bool m_shadeSmooth = true;

    Mesh(std::vector<Submesh>&& submeshes, bool hasNormals, bool hasUVs)
        : m_submeshes(submeshes), m_hasNormals(hasNormals), m_hasUVs(hasUVs) {
        for (auto& submesh : m_submeshes)
            submesh.bvh.build(submesh.faces);
    }

    HitRecord hit(Ray& ray) const override {
        HitRecord hit;

        // Check each submesh for intersection
        for (const auto& submesh : m_submeshes) {
            HitRecord submeshHit = submesh.bvh.intersect(ray);

            if (submeshHit.hit) {
                hit = submeshHit;
                hit.material = submesh.material;
            }
        }

        // Calculate interpolated normal and uv
        if (hit.hit) {
            assert(hit.face != nullptr);

            if (m_hasNormals && m_shadeSmooth) {
                const auto& face = *hit.face;
                vec3 interpolatedNormal = hit.barycentric.x * face.normals[0] + hit.barycentric.y * face.normals[1] + hit.barycentric.z * face.normals[2];
                hit.normal = glm::normalize(interpolatedNormal);
            }
            else {
                const auto& vertices = hit.face->vertices;
                vec3 flatNormal = glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]);
                hit.normal = glm::normalize(flatNormal);
            }

            if (m_hasUVs) {
                vec2 interpolatedUV = hit.barycentric.x * hit.face->uvs[0] + hit.barycentric.y * hit.face->uvs[1] + hit.barycentric.z * hit.face->uvs[2];
                hit.uv = interpolatedUV;
            }
            else
                hit.uv = vec2(0);
        }

        return hit;
    }

private:
    bool m_hasNormals;
    bool m_hasUVs;
};

class Model : public IHittable {
public:
    std::string m_name = "Unnamed";
    Transform m_transform;
    Ref<Mesh> m_mesh;

    Model(const Ref<Mesh>& mesh, const Transform& transform = Transform()) : m_mesh(mesh), m_transform(transform) {}

    HitRecord hit(Ray& ray) const override {
        Ray transformedRay = ray.createTransformedRay(m_transform.modelMatrixInverse());
        HitRecord hit = m_mesh->hit(transformedRay);

        if (hit.hit) {
            ray.updateFromTransformedRay(transformedRay, m_transform.modelMatrix());
            hit.transform(m_transform.modelMatrix());
        }

        return hit;
    }
};
