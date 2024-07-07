#pragma once

struct Face {
    std::array<vec3, 3> vertices;
    std::array<vec2, 3> uvs;
    std::array<vec3, 3> normals;
    vec3 tangent;
    vec3 bitangent;
};

// TODO split vertex and other data
/*
struct Vertex {
    vec3 position;
};

struct Triangle {
    std::array<u32, 3> vertices;
    vec3 tangent;
    vec3 bitangent;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    std::vector<vec3> normals;
    std::vector<vec2> uvs;
};
*/
