#pragma once

struct Face {
    std::array<vec3, 3> vertices; // TODO split vertex and other data
    std::array<vec2, 3> uvs;
    std::array<vec3, 3> normals;
};
