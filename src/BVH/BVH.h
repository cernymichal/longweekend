#pragma once

#include "HitRecord.h"
#include "Ray.h"

constexpr u32 BVH_MAX_DEPTH = 128;
constexpr u32 BVH_MAX_TRIANGLES_PER_LEAF = 32;

struct Triangle;

class BVH {
public:
    struct Stats {
        std::chrono::microseconds buildTime;
        u32 triangleCount = 0;
        u32 nodeCount = 0;
        u32 leafCount = 0;
        u32 maxDepth = 0;
        u32 maxTrianglesPerLeaf = 0;
    };

    HitRecord intersect(Ray& ray, bool backfaceCulling = true) const;

    void build(const std::vector<vec3>& vertices, std::vector<Triangle>& triangles);

    bool isBuilt() const { return m_vertices != nullptr; }

    const Stats& stats() const { return m_stats; }

private:
    struct Node {
        AABB aabb;
        u32 triangleCount;
        union {                 // Either triangleIndex or childIndex if triangleCount == 0
            u32 triangleIndex;  // First triangle
            u32 childIndex;     // Left child
        };
    };

    const std::vector<vec3>* m_vertices = nullptr;
    const std::vector<Triangle>* m_triangles = nullptr;
    std::vector<Node> m_nodes;

    Stats m_stats;
};
