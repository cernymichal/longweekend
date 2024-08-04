#pragma once

#include "HitRecord.h"
#include "Ray.h"

constexpr u32 BVH_MAX_DEPTH = 128;
constexpr u32 BVH_MAX_TRIANGLES_PER_LEAF = 32;

struct Triangle;

class BVH {
public:
    BVH(std::vector<vec3>& vertices, std::vector<Triangle>& triangles) : m_vertices(vertices), m_triangles(triangles) {}

    struct Stats {
        std::chrono::microseconds buildTime;
        u32 triangleCount = 0;
        u32 nodeCount = 0;
        u32 leafCount = 0;
        u32 maxDepth = 0;
        u32 maxTrianglesPerLeaf = 0;
    };

    HitRecord intersect(Ray& ray, bool backfaceCulling = true) const;

    void build(u32 perAxisSplitTests = 32);

    bool isBuilt() const { return !m_nodes.empty(); }

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

    struct SplitData {
        bool shouldSplit;
        u32 splitAxis;
        f32 splitPoint;
        AABB leftAABB;
        AABB rightAABB;
    };

    std::vector<vec3>& m_vertices;
    std::vector<Triangle>& m_triangles;
    std::vector<Node> m_nodes;
    u32 m_perAxisSplitTests = 8;

    Stats m_stats;

    bool splitNode(u32 nodeIndex, std::vector<AABB>& triangleAABBs);

    SplitData findBestSplit(u32 nodeIndex, const std::vector<AABB>& triangleAABBs) const;
};
