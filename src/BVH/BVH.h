#pragma once

#include "Face.h"
#include "Hittables/IHittable.h"

constexpr u32 BVH_MAX_DEPTH = 32;
constexpr u32 BVH_MAX_FACES_PER_LEAF = 32;

class BVH {
public:
    HitRecord intersect(Ray& ray) const;

    void build(std::vector<Face>& faces);

private:
    struct Node {
        AABB aabb;
        u32 faceIndex;
        u32 faceCount;
        u32 childIndex;  // TODO combine faceIndex and childIndex
    };

    std::vector<Face>* m_faces;
    std::vector<Node> m_nodes;
};
