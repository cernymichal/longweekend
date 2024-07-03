#pragma once

#include "Face.h"
#include "Hittables/IHittable.h"

constexpr u32 BVH_MAX_DEPTH = 32;
constexpr u32 BVH_MAX_FACES_PER_LEAF = 32;

class BVH {
public:
    HitRecord intersect(Ray& ray) const;

    void build(std::vector<Face>& faces);

    bool isBuilt() const { return !m_nodes.empty(); }

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
};
