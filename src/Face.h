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
    vec3 normal;
    vec2 uv;
};


struct Face {
    std::array<u32, 3> vertices;
    vec3 tangent;
    vec3 bitangent;
};

*/
