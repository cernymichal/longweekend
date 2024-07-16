#pragma once

#include "Face.h"
#include "Hittables/IHittable.h"

constexpr u32 BVH_MAX_DEPTH = 32;
constexpr u32 BVH_MAX_FACES_PER_LEAF = 32;

class BVH {
public:
    struct Stats {
        std::chrono::microseconds buildTime;
        u32 faceCount = 0;
        u32 nodeCount = 0;
        u32 leafCount = 0;
        u32 maxDepth = 0;
        u32 maxFacesPerLeaf = 0;
    };

    HitRecord intersect(Ray& ray, bool backfaceCulling = true) const;

    void build(std::vector<Face>& faces);

    bool isBuilt() const { return !m_nodes.empty(); }

    const Stats& stats() const { return m_stats; }

private:
    struct Node {
        AABB aabb;
        u32 faceCount;
        union {              // Either faceIndex or childIndex if faceCount == 0
            u32 faceIndex;   // First face
            u32 childIndex;  // Left child
        };
    };

    std::vector<Face>* m_faces;
    std::vector<Node> m_nodes;

    Stats m_stats;
};
