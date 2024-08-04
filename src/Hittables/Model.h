#pragma once

#include "IHittable.h"
#include "Material.h"
#include "Mesh.h"

class Model : public IHittable {
public:
    std::string m_name = "Unnamed";
    Mesh m_mesh;
    bool m_backfaceCulling = true;

    Model(Mesh&& mesh) : m_mesh(mesh) {}

    HitRecord hit(Ray& ray) const override {
        // Find closest intersection
        HitRecord hit = m_mesh.geometry->bvh.intersect(ray, m_backfaceCulling);

        // Calculate interpolated normal and uv
        if (hit.hit) {
            assert(hit.triangleId >= 0 && hit.triangleId < m_mesh.geometry->triangles.size());

            hit.material = m_mesh.materials[m_mesh.geometry->triangles[hit.triangleId].materialId];
            hit.geometry = m_mesh.geometry;

            const auto& triangle = m_mesh.geometry->triangles[hit.triangleId];
            const auto& vertexIds = triangle.vertexIds;
            const auto& vertices = m_mesh.geometry->vertices;
            const auto& uvs = m_mesh.geometry->uvs;
            const auto& normals = m_mesh.geometry->normals;
            const auto& tangents = m_mesh.geometry->tangents;

            if (!uvs.empty()) {
                vec2 interpolatedUV = hit.barycentric.x * uvs[vertexIds[0]] + hit.barycentric.y * uvs[vertexIds[1]] + hit.barycentric.z * uvs[vertexIds[2]];
                hit.uv = interpolatedUV;
            }

            if (!normals.empty()) {
                vec3 interpolatedNormal = hit.barycentric.x * normals[vertexIds[0]] + hit.barycentric.y * normals[vertexIds[1]] + hit.barycentric.z * normals[vertexIds[2]];
                hit.normal = glm::normalize(interpolatedNormal);
            }
            else {
                vec3 flatNormal = glm::cross(vertices[vertexIds[1]] - vertices[vertexIds[0]], vertices[vertexIds[2]] - vertices[vertexIds[0]]);
                hit.normal = glm::normalize(flatNormal);
            }

            if (!tangents.empty()) {
                vec3 interpolatedTangent = hit.barycentric.x * vec3(tangents[vertexIds[0]]) + hit.barycentric.y * vec3(tangents[vertexIds[1]]) + hit.barycentric.z * vec3(tangents[vertexIds[2]]);
                f32 handedness = tangents[vertexIds[0]].w;

                hit.tangent = glm::normalize(interpolatedTangent - glm::dot(interpolatedTangent, hit.normal) * hit.normal);  // Reorthogonalize after normal interpolation
                hit.bitangent = glm::normalize(handedness * glm::cross(hit.normal, hit.tangent));
            }
        }

        return hit;
    }

    void frameBegin() override {
        if (!m_mesh.geometry->bvh.isBuilt()) {  // TODO paralelize - mutex in bvh
            m_mesh.geometry->bvh.build();

            const auto& stats = m_mesh.geometry->bvh.stats();
            LOG(std::format(
                "{} BVH:\n\tbuildTime\t\t= {}ms\n\ttriangleCount\t\t= {}\n\tnodeCount\t\t= {}\n\tleafCount\t\t= {}\n\tmaxDepth\t\t= {}\n\tavgTrianglesPerLeaf\t= {}\n\tmaxTrianglesPerLeaf\t= {}",
                m_name,
                stats.buildTime.count() / 1000.0f,
                stats.triangleCount,
                stats.nodeCount,
                stats.leafCount,
                stats.maxDepth,
                (f32)stats.triangleCount / stats.leafCount,
                stats.maxTrianglesPerLeaf));
        }

        for (const auto& material : m_mesh.materials)
            m_backfaceCulling &= material->backfaceCulling && material->scatterFunction != dielectricScatter;
    }
};
