#include "MeshIO.h"

#include <fstream>

#include "Material.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

Mesh loadOBJ(const std::filesystem::path& filePath) {
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

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();
    std::vector<Ref<Material>> materialRefs;

    for (const auto& material : materials) {
        auto mat = makeRef<LambertianMaterial>();
        mat->m_albedo = std::bit_cast<vec3>(material.diffuse);
        materialRefs.push_back(mat);
    }

    std::vector<Mesh::Face> faces;

    bool hasNormals = !attrib.normals.empty();
    bool hasTexcoords = !attrib.texcoords.empty();

    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            auto& face = faces.emplace_back();
            // face.material = materialRefs[shapes[s].mesh.material_ids[f]]; TODO submeshes

            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            assert(fv == 3);

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
                face.vertices[v] = vec3(vx, vy, vz);

                if (hasNormals) {
                    // TODO per-vertex normals? normal maps?
                    /*
                    assert(idx.normal_index >= 0);
                    tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                    face.normal = vec3(nx, ny, nz);
                    */
                }

                if (hasTexcoords) {
                    assert(idx.texcoord_index >= 0);
                    tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                    face.uvs[v] = vec2(tx, ty);
                }
                else // Set dummy UVs
                    face.uvs[v] = vec2(0);
            }

            if (!hasNormals) // Calculate normals if they are not provided
                face.normal = glm::normalize(glm::cross(face.vertices[1] - face.vertices[0], face.vertices[2] - face.vertices[0]));

            index_offset += fv;
        }
    }

    return Mesh(std::move(faces), materialRefs.size() > 0 ? materialRefs[0] : nullptr);
}