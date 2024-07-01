#pragma once

struct Face {
    std::array<vec3, 3> vertices;
    std::array<vec2, 3> uvs;
    // std::array<vec3, 3> normals; // TODO per-vertex normals
    vec3 normal;
};
