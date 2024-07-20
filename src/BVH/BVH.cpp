#include "BVH.h"

#include "Mesh.h"

HitRecord BVH::intersect(Ray& ray, bool backfaceCulling) const {
    HitRecord hit;

    if (m_nodes.empty())
        return hit;

    RayShearConstants raySheerConstants(ray.direction);

    std::array<u32, BVH_MAX_DEPTH> stack;
    u32 stackSize = 0;
    stack[stackSize++] = 0;

    while (stackSize != 0) {
        u32 nodeIndex = stack[--stackSize];
        const Node& node = m_nodes[nodeIndex];

#ifdef BVH_TEST
        ray.aabbTestCount++;
#endif

        auto nodeIntersection = rayAABBintersection(ray.origin, ray.invDirection, node.aabb);
        if (std::isnan(nodeIntersection.min) || !ray.tInterval.intersects(nodeIntersection))
            continue;

        if (node.triangleCount != 0) {
            // Leaf node
            for (u32 i = node.triangleIndex; i < node.triangleIndex + node.triangleCount; i++) {
                const Triangle& triangle = m_triangles->at(i);

#ifdef BVH_TEST
                ray.triangleTestCount++;
#endif

                const auto& vertexIds = triangle.vertexIds;
                auto [t, barycentric] = rayTriangleIntersectionWT(ray.origin, raySheerConstants, m_vertices->at(vertexIds[0]), m_vertices->at(vertexIds[1]), m_vertices->at(vertexIds[2]), backfaceCulling);
                if (!std::isnan(t) && ray.tInterval.surrounds(t)) {
                    hit.hit = true;
                    hit.triangleId = i;
                    hit.barycentric = barycentric;
                    ray.tInterval.max = t;
                }
            }

            continue;
        }

        stack[stackSize++] = node.childIndex;
        stack[stackSize++] = node.childIndex + 1;

        /*
        // Add children to stack sorted by tNear
        const Node& leftNode = m_nodes[node.childIndex];
        auto leftNodeIntersection = rayAABBintersection(ray.origin, ray.invDirection, leftNode.aabb);
        bool leftNodeHit = !std::isnan(leftNodeIntersection.min) && ray.tInterval.intersects(leftNodeIntersection);

        const Node& rightNode = m_nodes[node.childIndex + 1];
        auto rightNodeIntersection = rayAABBintersection(ray.origin, ray.invDirection, rightNode.aabb);
        bool rightNodeHit = !std::isnan(rightNodeIntersection.min) && ray.tInterval.intersects(rightNodeIntersection);

        if (leftNodeHit && rightNodeHit) {
            if (leftNodeIntersection.min < rightNodeIntersection.min) {
                stack[stackSize++] = node.childIndex;
                stack[stackSize++] = node.childIndex + 1;
            }
            else {
                stack[stackSize++] = node.childIndex + 1;
                stack[stackSize++] = node.childIndex;
            }
        }
        else if (leftNodeHit)
            stack[stackSize++] = node.childIndex;
        else if (rightNodeHit)
            stack[stackSize++] = node.childIndex + 1;
        */
    }

    return hit;
}

void BVH::build(const std::vector<vec3>& vertices, std::vector<Triangle>& triangles) {
    m_stats = Stats();
    m_stats.triangleCount = (u32)triangles.size();
    auto start = std::chrono::high_resolution_clock::now();

    m_vertices = &vertices;
    m_triangles = &triangles;
    m_nodes.clear();
    m_nodes.reserve(std::bit_ceil(triangles.size() / BVH_MAX_TRIANGLES_PER_LEAF + 1) * 2 - 1);  // leaftCount * 2 - 1

    // Calculate AABBs for each triangle
    std::vector<AABB> triangleAABBs;
    triangleAABBs.reserve(triangles.size());
    for (const Triangle& triangle : triangles) {
        AABB aabb = AABB::empty();
        for (u32 i = 0; i < 3; i++)
            aabb = aabb.extendTo(vertices[triangle.vertexIds[i]]);
        triangleAABBs.push_back(aabb);
    }

    m_nodes.push_back({
        .aabb = AABB::empty(),
        .triangleCount = (u32)triangles.size(),
        .triangleIndex = 0,
    });

    std::queue<std::pair<u32, u32>> queue;
    queue.push({0, 1});

    while (!queue.empty()) {
        u32 currentNodeIndex = queue.front().first;
        Node& currentNode = m_nodes[currentNodeIndex];
        u32 depth = queue.front().second;
        m_stats.maxDepth = std::max(m_stats.maxDepth, depth);
        queue.pop();

        // Calculate node AABB
        currentNode.aabb = AABB::empty();
        for (u32 i = currentNode.triangleIndex; i < currentNode.triangleIndex + currentNode.triangleCount; i++)
            currentNode.aabb = currentNode.aabb.boundingUnion(triangleAABBs[i]);

        // Dont split if we have too few triangles or reached max depth
        if (currentNode.triangleCount <= BVH_MAX_TRIANGLES_PER_LEAF || depth >= BVH_MAX_DEPTH) {
            m_stats.leafCount++;
            m_stats.maxTrianglesPerLeaf = std::max(m_stats.maxTrianglesPerLeaf, currentNode.triangleCount);
            continue;
        }

        // Find longest axis

        u32 longestAxis = 0;
        f32 axisLength = currentNode.aabb.max[0] - currentNode.aabb.min[0];
        for (u32 i = 1; i < 3; i++) {
            f32 length = currentNode.aabb.max[i] - currentNode.aabb.min[i];
            if (length > axisLength) {
                longestAxis = i;
                axisLength = length;
            }
        }

        // Split triangles along longest axis
        f32 splitPoint = currentNode.aabb.min[longestAxis] + axisLength / 2;
        u32 j = currentNode.triangleIndex + currentNode.triangleCount - 1;
        for (u32 i = currentNode.triangleIndex; i < j;) {
            if (triangleAABBs[i].min[longestAxis] < splitPoint)
                i++;
            else {
                std::swap(triangles[i], triangles[j]);
                std::swap(triangleAABBs[i], triangleAABBs[j]);
                j--;
            }
        }

        // Split node
        Node leftChild = {
            .triangleCount = j - currentNode.triangleIndex,
            .triangleIndex = currentNode.triangleIndex,
        };

        Node rightChild = {
            .triangleCount = currentNode.triangleCount - leftChild.triangleCount,
            .triangleIndex = j,
        };

        if (leftChild.triangleCount == 0 || rightChild.triangleCount == 0) {
            m_stats.leafCount++;
            m_stats.maxTrianglesPerLeaf = std::max(m_stats.maxTrianglesPerLeaf, currentNode.triangleCount);
            continue;  // TODO handle this case, try another axis?
        }

        currentNode.triangleCount = 0;
        currentNode.childIndex = (u32)m_nodes.size();

        queue.push({currentNode.childIndex, depth + 1});
        queue.push({currentNode.childIndex + 1, depth + 1});

        // BEWARE this could invalidate node references on resize!
        m_nodes.push_back(leftChild);
        m_nodes.push_back(rightChild);
    }

    m_nodes.shrink_to_fit();

    auto end = std::chrono::high_resolution_clock::now();
    m_stats.buildTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    m_stats.nodeCount = (u32)m_nodes.size();
}
