#include "MeshIO.h"

#include <fstream>

#include "Material.h"
#include "TextureIO.h"

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
        auto material = makeRef<Material>();
        *material = {
            .name = mtlMaterial.name,
            .albedo = std::bit_cast<vec3>(mtlMaterial.diffuse),
            .emission = std::bit_cast<vec3>(mtlMaterial.emission),
            .emissionIntensity = material->emission != vec3(0) ? 1.0f : 0.0f,
        };

        // TODO support texture options
        // TODO share textures
        if (!mtlMaterial.diffuse_texname.empty())
            material->albedoTexture = makeRef<Texture<vec3>>(loadTexture<vec3>(filePath.parent_path() / mtlMaterial.diffuse_texname, true));

        if (!mtlMaterial.emissive_texname.empty())
            material->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>(filePath.parent_path() / mtlMaterial.emissive_texname, true));

        if (!mtlMaterial.normal_texname.empty())
            material->normalTexture = makeRef<Texture<vec3>>(loadTexture<vec3>(filePath.parent_path() / mtlMaterial.normal_texname, true));
        else if (!mtlMaterial.bump_texname.empty())  // TODO check if bump contains 3 channels
            material->normalTexture = makeRef<Texture<vec3>>(loadTexture<vec3>(filePath.parent_path() / mtlMaterial.bump_texname, true));

        auto& submesh = submeshes.emplace_back();
        submesh.material = material;
    }

    for (size_t shapeId = 0; shapeId < shapes.size(); shapeId++) {
        auto& mesh = shapes[shapeId].mesh;
        for (size_t faceId = 0; faceId < mesh.num_face_vertices.size(); faceId++) {
            i32 materialId = mesh.material_ids[faceId];
            auto& face = (materialId >= 0 ? submeshes[materialId] : noMaterialSubmesh).faces.emplace_back();

            // Load face vertices
            assert(mesh.num_face_vertices[faceId] == 3);
            for (size_t vertexId = 0; vertexId < 3; vertexId++) {
                tinyobj::index_t idx = mesh.indices[3 * faceId + vertexId];

                face.vertices[vertexId] = vertices[idx.vertex_index];
                face.normals[vertexId] = idx.normal_index >= 0 ? normals[idx.normal_index] : vec3(0);
                face.uvs[vertexId] = idx.texcoord_index >= 0 ? texcoords[idx.texcoord_index] : vec2(0);
            }

            // Calculate tangent and bitangent
            if (hasUVs) {
                vec3 edge1 = face.vertices[1] - face.vertices[0];
                vec3 edge2 = face.vertices[2] - face.vertices[0];
                vec2 deltaUV1 = face.uvs[1] - face.uvs[0];
                vec2 deltaUV2 = face.uvs[2] - face.uvs[0];

                f32 determinantInv = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

                face.tangent = glm::normalize(determinantInv * (deltaUV2.y * edge1 - deltaUV1.y * edge2));
                face.bitangent = glm::normalize(determinantInv * (-deltaUV2.x * edge1 + deltaUV1.x * edge2));
            }
        }
    }

    if (noMaterialSubmesh.faces.size() > 0) {
        LOG("Mesh has faces without material");
        noMaterialSubmesh.material = makeRef<Material>();
        submeshes.push_back(noMaterialSubmesh);
    }

    Mesh mesh(std::move(submeshes), hasNormals, hasUVs);
    mesh.m_name = filePath.stem().string();
    mesh.m_shadeSmooth = hasNormals;

    return mesh;
}
