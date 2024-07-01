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
        : m_submeshes(submeshes), m_hasNormals(hasNormals), m_hasUVs(hasUVs) {}

    HitRecord hit(const Ray& ray, Interval<f32> tInterval) const override {
        HitRecord hit{false};
        for (const auto& submesh : m_submeshes) {
            for (const auto& face : submesh.faces) {
                auto [t, barycentric] = rayTriangleIntersectionCoordinates(ray.origin, ray.direction, face.vertices);

                if (!isnan(t) && tInterval.surrounds(t)) {
                    hit.hit = true;
                    hit.t = t;
                    tInterval.max = hit.t;
                    hit.point = ray.at(t);

                    if (m_hasNormals && m_shadeSmooth) {
                        vec3 interpolatedNormal = barycentric.x * face.normals[0] + barycentric.y * face.normals[1] + barycentric.z * face.normals[2];
                        hit.setNormal(ray, glm::normalize(interpolatedNormal));
                    }
                    else {
                        vec3 flatNormal = glm::cross(face.vertices[1] - face.vertices[0], face.vertices[2] - face.vertices[0]);
                        hit.setNormal(ray, glm::normalize(flatNormal));
                    }

                    hit.material = submesh.material;

                    if (m_hasUVs) {
                        vec2 interpolatedUV = barycentric.x * face.uvs[0] + barycentric.y * face.uvs[1] + barycentric.z * face.uvs[2];
                        hit.uv = interpolatedUV;
                    }
                    else
                        hit.uv = vec2(0);
                }
            }
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

    HitRecord hit(const Ray& ray, Interval<f32> tInterval) const override {
        auto transformedRay = m_transform.modelMatrixInverse() * ray;

        auto transformedTInterval = tInterval;
        auto transformedDirectionLength = glm::length(transformedRay.direction);
        if (!isinf(tInterval.min)) {
            vec3 tMinVec = m_transform.modelMatrixInverse() * vec4(tInterval.min * ray.direction, 0);
            transformedTInterval.min = glm::length(tMinVec) / transformedDirectionLength;
        }
        if (!isinf(tInterval.max)) {
            vec3 tMaxVec = m_transform.modelMatrixInverse() * vec4(tInterval.max * ray.direction, 0);
            transformedTInterval.max = glm::length(tMaxVec) / transformedDirectionLength;
        }

        HitRecord hit = m_mesh->hit(transformedRay, transformedTInterval);

        if (hit.hit) {
            hit.point = m_transform.modelMatrix() * vec4(hit.point, 1);
            hit.t = glm::length(hit.point - ray.origin) / glm::length(ray.direction);  // TODO verify this
            hit.setNormal(ray, glm::normalize(vec3(m_transform.modelMatrix() * vec4(hit.normal, 0))));
        }

        return hit;
    }
};
