#include "MeshIO.h"

#include <fstream>

#include "Material.h"
#include "TextureIO.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

Model loadOBJ(const std::filesystem::path& filePath) {
    LOG("Loading mesh " << filePath);

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

    auto& loadedAttributes = reader.GetAttrib();
    auto& loadedShapes = reader.GetShapes();
    auto& loadedMaterials = reader.GetMaterials();

    static_assert(sizeof(vec3) == 3 * sizeof(float));
    static_assert(sizeof(vec2) == 2 * sizeof(float));
    const vec3* loadedVertices = reinterpret_cast<const vec3*>(loadedAttributes.vertices.data());
    const vec3* loadedNormals = reinterpret_cast<const vec3*>(loadedAttributes.normals.data());
    const vec2* loadedUVs = reinterpret_cast<const vec2*>(loadedAttributes.texcoords.data());

    bool hasNormals = !loadedAttributes.normals.empty();
    bool hasUVs = !loadedAttributes.texcoords.empty();
    bool calculateTangents = hasNormals && hasUVs;

    Mesh modelMesh;

    // Load materials
    for (const auto& loadedMaterial : loadedMaterials) {
        auto material = makeRef<Material>();
        *material = {
            .name = loadedMaterial.name,
            .albedo = std::bit_cast<vec3>(loadedMaterial.diffuse),
            .emission = std::bit_cast<vec3>(loadedMaterial.emission),
            .emissionIntensity = material->emission != vec3(0) ? 1.0f : 0.0f,
        };

        // TODO support texture options
        // TODO share textures
        if (!loadedMaterial.diffuse_texname.empty())
            material->albedoTexture = makeRef<Texture<vec3>>(loadTexture<vec3>(filePath.parent_path() / loadedMaterial.diffuse_texname, true));

        if (!loadedMaterial.emissive_texname.empty())
            material->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>(filePath.parent_path() / loadedMaterial.emissive_texname, true));

        if (!loadedMaterial.normal_texname.empty())
            material->normalTexture = makeRef<Texture<vec3>>(loadTexture<vec3>(filePath.parent_path() / loadedMaterial.normal_texname, true));
        else if (!loadedMaterial.bump_texname.empty())  // TODO check if bump contains 3 channels
            material->normalTexture = makeRef<Texture<vec3>>(loadTexture<vec3>(filePath.parent_path() / loadedMaterial.bump_texname, true));

        if (!loadedMaterial.alpha_texname.empty()) {
            material->alphaTexture = makeRef<Texture<f32>>(loadTexture<f32>(filePath.parent_path() / loadedMaterial.alpha_texname, true));
            material->backfaceCulling = false;
        }

        modelMesh.materials.push_back(material);
    }

    // Load attributes and construct the triangle index buffer
    modelMesh.geometry = makeRef<MeshGeometry>();

    u32 vertexCount = loadedAttributes.vertices.size() / 3;
    std::vector<std::vector<u32>> vertexIndices(vertexCount);

    std::vector<vec3> bitangents;  // Temporary storage for bitangents, used to calculate handedness of the tangent basis

    bool hasNoMaterialTriangles = false;

    for (size_t shapeId = 0; shapeId < loadedShapes.size(); shapeId++) {
        auto& mesh = loadedShapes[shapeId].mesh;
        for (size_t triangleId = 0; triangleId < mesh.num_face_vertices.size(); triangleId++) {
            auto& triangle = modelMesh.geometry->triangles.emplace_back();

            // Load material
            i32 materialId = mesh.material_ids[triangleId];
            if (materialId < 0) {
                // No material assigned to this triangle, use default material added at the end
                hasNoMaterialTriangles = true;
                materialId = modelMesh.materials.size();
            }
            triangle.materialId = materialId;

            // Load triangle loadedVertices
            assert(mesh.num_face_vertices[triangleId] == 3);
            for (size_t vertexId = 0; vertexId < 3; vertexId++) {
                tinyobj::index_t idx = mesh.indices[3 * triangleId + vertexId];

                // Find or create vertex
                u32 vertexIndex = u32(-1);
                const vec3& vertexPosition = loadedVertices[idx.vertex_index];
                const vec2& vertexUV = hasUVs && idx.texcoord_index >= 0 ? loadedUVs[idx.texcoord_index] : vec2(0);
                const vec3& vertexNormal = hasNormals && idx.normal_index >= 0 ? loadedNormals[idx.normal_index] : vec3(0);

                for (u32 index : vertexIndices[idx.vertex_index]) {
                    bool hasSamePosition = true;  // modelMesh.geometry->vertices[index] == vertexPosition;  // This is already satisfied by using the same index
                    bool hasSameUV = !hasUVs || modelMesh.geometry->uvs[index] == vertexUV;
                    bool hasSameNormal = !hasNormals || modelMesh.geometry->normals[index] == vertexNormal;

                    if (false && hasSamePosition && hasSameUV && hasSameNormal) {
                        vertexIndex = index;
                        break;
                    }
                }

                if (vertexIndex == u32(-1)) {
                    // Equal vertex was not found, create a new one

                    modelMesh.geometry->vertices.push_back(vertexPosition);
                    if (hasUVs)
                        modelMesh.geometry->uvs.push_back(vertexUV);
                    if (hasNormals)
                        modelMesh.geometry->normals.push_back(vertexNormal);
                    if (calculateTangents) {
                        modelMesh.geometry->tangents.push_back(vec4(0));
                        bitangents.push_back(vec3(0));
                    }

                    vertexIndex = modelMesh.geometry->vertices.size() - 1;
                    vertexIndices[idx.vertex_index].push_back(vertexIndex);
                }

                triangle.vertexIds[vertexId] = vertexIndex;
            }

            // Calculate tangent and bitangent
            if (calculateTangents) {
                vec3 edge1 = modelMesh.geometry->vertices[triangle.vertexIds[1]] - modelMesh.geometry->vertices[triangle.vertexIds[0]];
                vec3 edge2 = modelMesh.geometry->vertices[triangle.vertexIds[2]] - modelMesh.geometry->vertices[triangle.vertexIds[0]];
                vec2 deltaUV1 = modelMesh.geometry->uvs[triangle.vertexIds[1]] - modelMesh.geometry->uvs[triangle.vertexIds[0]];
                vec2 deltaUV2 = modelMesh.geometry->uvs[triangle.vertexIds[2]] - modelMesh.geometry->uvs[triangle.vertexIds[0]];

                f32 determinantInv = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

                vec3 tangent = glm::normalize(determinantInv * (deltaUV2.y * edge1 - deltaUV1.y * edge2));
                vec3 bitangent = glm::normalize(determinantInv * (-deltaUV2.x * edge1 + deltaUV1.x * edge2));

                for (size_t i = 0; i < 3; i++) {
                    modelMesh.geometry->tangents[triangle.vertexIds[i]] += vec4(tangent, 0);
                    bitangents[triangle.vertexIds[i]] += bitangent;
                }
            }
        }
    }

    if (hasNoMaterialTriangles) {
        // Add default material for triangles without one
        LOG("Mesh has triangles without material, adding default material");
        modelMesh.materials.push_back(makeRef<Material>());
    }

    // Calculate per vertex tangents
    if (calculateTangents) {
        for (size_t i = 0; i < modelMesh.geometry->vertices.size(); i++) {
            vec3 tangent = vec3(modelMesh.geometry->tangents[i]);
            const vec3& bitangent = bitangents[i];
            const vec3& normal = modelMesh.geometry->normals[i];

            // Gram-Schmidt orthogonalization
            tangent = glm::normalize(tangent - glm::dot(tangent, normal) * normal);

            // Calculate handedness
            f32 handedness = glm::dot(glm::cross(normal, tangent), bitangent) < 0.0f ? -1.0f : 1.0f;
            // bitangent = glm::normalize(handedness * glm::cross(normal, tangent));

            modelMesh.geometry->tangents[i] = vec4(tangent, handedness);
        }
    }

    modelMesh.geometry->vertices.shrink_to_fit();
    modelMesh.geometry->uvs.shrink_to_fit();
    modelMesh.geometry->normals.shrink_to_fit();
    modelMesh.geometry->tangents.shrink_to_fit();
    modelMesh.geometry->triangles.shrink_to_fit();

    Model model(std::move(modelMesh));
    model.m_name = filePath.stem().string();

    return model;
}
