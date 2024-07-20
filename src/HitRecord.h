#pragma once

struct Material;
struct MeshGeometry;

struct HitRecord {
    // Set by the hit function
    bool hit = false;
    vec2 uv = vec2(0);
    vec3 normal = vec3(0);
    vec3 tangent = vec3(0);
    vec3 bitangent = vec3(0);
    vec3 barycentric = vec3(0);
    Ref<const Material> material;

    vec3 point;  // Set just before scattering

    u32 triangleId = u32(-1);
    Ref<const MeshGeometry> geometry;

    void transform(const mat4& transform) {
        normal = glm::normalize(vec3(transform * vec4(normal, 0)));
        tangent = glm::normalize(vec3(transform * vec4(tangent, 0)));
        bitangent = glm::normalize(vec3(transform * vec4(bitangent, 0)));
    }
};
