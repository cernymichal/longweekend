#include "MeshIO.h"

#include <fstream>

#include "Material.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

Mesh loadOBJ(const std::filesystem::path& filePath) {
    LOG("Loading meshes from " << filePath);

    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filePath.string(), reader_config)) {
        if (!reader.Error().empty())
            LOG("tinyobjloader: " << reader.Error());

        throw std::runtime_error("Failed to load mesh");
    }

    if (!reader.Warning().empty())
        LOG("tinyobjloader: " << reader.Warning());

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& mtlMaterials = reader.GetMaterials();

    static_assert(sizeof(vec3) == 3 * sizeof(float));
    static_assert(sizeof(vec2) == 2 * sizeof(float));
    const vec3* vertices = reinterpret_cast<const vec3*>(attrib.vertices.data());
    const vec3* normals = reinterpret_cast<const vec3*>(attrib.normals.data());
    const vec2* texcoords = reinterpret_cast<const vec2*>(attrib.texcoords.data());

    std::vector<Mesh::Submesh> submeshes;
    Mesh::Submesh noMaterialSubmesh;
    bool hasNormals = !attrib.normals.empty();
    bool hasUVs = !attrib.texcoords.empty();

    for (const auto& mtlMaterial : mtlMaterials) {
        auto material = makeRef<LambertianMaterial>();
        material->m_name = mtlMaterial.name;
        material->m_albedo = std::bit_cast<vec3>(mtlMaterial.diffuse);
        material->m_emission = std::bit_cast<vec3>(mtlMaterial.emission);
        material->m_emissionIntensity = material->m_emission != vec3(0) ? 1.0f : 0.0f;

        auto& submesh = submeshes.emplace_back();
        submesh.material = material;
    }

    for (size_t shapeId = 0; shapeId < shapes.size(); shapeId++) {
        auto& mesh = shapes[shapeId].mesh;
        for (size_t faceId = 0; faceId < mesh.num_face_vertices.size(); faceId++) {
            i32 materialId = mesh.material_ids[faceId];
            auto& face = (materialId >= 0 ? submeshes[materialId] : noMaterialSubmesh).faces.emplace_back();

            assert(mesh.num_face_vertices[faceId] == 3);
            for (size_t vertexId = 0; vertexId < 3; vertexId++) {
                tinyobj::index_t idx = mesh.indices[3 * faceId + vertexId];

                face.vertices[vertexId] = vertices[idx.vertex_index];
                face.normals[vertexId] = idx.normal_index >= 0 ? normals[idx.normal_index] : vec3(0);
                face.uvs[vertexId] = idx.texcoord_index >= 0 ? texcoords[idx.texcoord_index] : vec2(0);
            }
        }
    }

    if (noMaterialSubmesh.faces.size() > 0) {
        LOG("Mesh has faces without material");
        noMaterialSubmesh.material = makeRef<LambertianMaterial>();
        submeshes.push_back(noMaterialSubmesh);
    }

    Mesh mesh(std::move(submeshes), hasNormals, hasUVs);
    mesh.m_name = filePath.stem().string();
    mesh.m_shadeSmooth = hasNormals;

    return mesh;
}
