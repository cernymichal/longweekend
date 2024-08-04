#pragma once

#include "BVH/BVH.h"

struct Material;

struct Triangle {
    std::array<u32, 3> vertexIds;
    u32 materialId;
};

struct MeshGeometry {
    std::vector<vec3> vertices;
    std::vector<vec2> uvs;
    std::vector<vec3> normals;
    std::vector<vec4> tangents;  // xyz = tangent, w = handedness

    std::vector<Triangle> triangles;

    BVH bvh;

    MeshGeometry() : bvh(vertices, triangles) {}
};

struct Mesh {
    Ref<MeshGeometry> geometry;
    std::vector<Ref<Material>> materials;
};
