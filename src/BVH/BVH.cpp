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
        if (std::isnan(nodeIntersection.min) || ray.tInterval.intersection(nodeIntersection).length() < 0)
            continue;

        if (node.triangleCount != 0) {
            // Leaf node
            for (u32 i = node.triangleIndex; i < node.triangleIndex + node.triangleCount; i++) {
                const Triangle& triangle = m_triangles.at(i);

#ifdef BVH_TEST
                ray.triangleTestCount++;
#endif

                const auto& vertexIds = triangle.vertexIds;
                auto [t, barycentric] = rayTriangleIntersectionWT(ray.origin, raySheerConstants, m_vertices[vertexIds[0]], m_vertices[vertexIds[1]], m_vertices[vertexIds[2]], backfaceCulling);
                if (!std::isnan(t) && ray.tInterval.surrounds(t)) {
                    hit.hit = true;
                    hit.triangleId = i;
                    hit.barycentric = barycentric;
                    ray.tInterval.max = t;
                }
            }

            continue;
        }

        // Add children to stack sorted by tNear
        const Node& leftNode = m_nodes[node.childIndex];
        auto leftNodeIntersection = rayAABBintersection(ray.origin, ray.invDirection, leftNode.aabb);
        bool leftNodeHit = !std::isnan(leftNodeIntersection.min) && ray.tInterval.intersection(leftNodeIntersection).length() >= 0;

        const Node& rightNode = m_nodes[node.childIndex + 1];
        auto rightNodeIntersection = rayAABBintersection(ray.origin, ray.invDirection, rightNode.aabb);
        bool rightNodeHit = !std::isnan(rightNodeIntersection.min) && ray.tInterval.intersection(rightNodeIntersection).length() >= 0;

        if (leftNodeHit && rightNodeHit) {
            if (leftNodeIntersection.min < rightNodeIntersection.min) {
                stack[stackSize++] = node.childIndex + 1;
                stack[stackSize++] = node.childIndex;
            }
            else {
                stack[stackSize++] = node.childIndex;
                stack[stackSize++] = node.childIndex + 1;
            }
        }
        else if (leftNodeHit)
            stack[stackSize++] = node.childIndex;
        else if (rightNodeHit)
            stack[stackSize++] = node.childIndex + 1;
    }

    return hit;
}

void BVH::build(u32 perAxisSplitTests) {
    m_perAxisSplitTests = perAxisSplitTests;
    m_stats = Stats();
    m_stats.triangleCount = (u32)m_triangles.size();
    auto start = std::chrono::high_resolution_clock::now();

    m_nodes.clear();
    m_nodes.reserve(std::bit_ceil(m_triangles.size() / BVH_MAX_TRIANGLES_PER_LEAF + 1) * 2 - 1);  // leaftCount * 2 - 1

    // Pre-calculate AABBs for each triangle
    std::vector<AABB> triangleAABBs;
    triangleAABBs.reserve(m_triangles.size());
    for (const Triangle& triangle : m_triangles) {
        AABB aabb = AABB::empty();
        for (u32 i = 0; i < 3; i++)
            aabb = aabb.extendTo(m_vertices[triangle.vertexIds[i]]);
        triangleAABBs.push_back(aabb);
    }

    // Create root node
    Node rootNode = {
        .aabb = AABB::empty(),
        .triangleCount = (u32)m_triangles.size(),
        .triangleIndex = 0,
    };
    for (u32 i = rootNode.triangleIndex; i < rootNode.triangleIndex + rootNode.triangleCount; i++)
        rootNode.aabb = rootNode.aabb.boundingUnion(triangleAABBs[i]);

    m_nodes.push_back(rootNode);

    std::queue<std::pair<u32, u32>> queue;
    queue.push({0, 1});

    while (!queue.empty()) {
        u32 currentNodeIndex = queue.front().first;
        u32 depth = queue.front().second;
        m_stats.maxDepth = std::max(m_stats.maxDepth, depth);
        queue.pop();

        // Split Node
        bool shouldSplit = m_nodes[currentNodeIndex].triangleCount > BVH_MAX_TRIANGLES_PER_LEAF && depth < BVH_MAX_DEPTH;
        bool splitSuccess = shouldSplit ? splitNode(currentNodeIndex, triangleAABBs) : false;
        if (!splitSuccess) {
            m_stats.leafCount++;
            m_stats.maxTrianglesPerLeaf = std::max(m_stats.maxTrianglesPerLeaf, m_nodes[currentNodeIndex].triangleCount);
            continue;
        }

        // Add children to queue
        queue.push({m_nodes[currentNodeIndex].childIndex, depth + 1});
        queue.push({m_nodes[currentNodeIndex].childIndex + 1, depth + 1});
    }

    m_nodes.shrink_to_fit();

    auto end = std::chrono::high_resolution_clock::now();
    m_stats.buildTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    m_stats.nodeCount = (u32)m_nodes.size();
}

bool BVH::splitNode(u32 nodeIndex, std::vector<AABB>& triangleAABBs) {
    Node& parentNode = m_nodes[nodeIndex];

    // Split triangles
    BVH::SplitData split = findBestSplit(nodeIndex, triangleAABBs);
    if (!split.shouldSplit)
        return false;

    // Sort triangles based on split
    u32 j = parentNode.triangleIndex + parentNode.triangleCount - 1;
    for (u32 i = parentNode.triangleIndex; i <= j;) {
        if (triangleAABBs[i].center()[split.splitAxis] < split.splitPoint)
            i++;
        else {
            std::swap(m_triangles[i], m_triangles[j]);
            std::swap(triangleAABBs[i], triangleAABBs[j]);
            j--;
        }
    }

    // Split node
    Node leftChild = {
        .aabb = split.leftAABB,
        .triangleCount = j - parentNode.triangleIndex + 1,
        .triangleIndex = parentNode.triangleIndex,
    };

    Node rightChild = {
        .aabb = split.rightAABB,
        .triangleCount = parentNode.triangleCount - leftChild.triangleCount,
        .triangleIndex = leftChild.triangleIndex + leftChild.triangleCount,
    };

    parentNode.triangleCount = 0;
    parentNode.childIndex = (u32)m_nodes.size();

    // BEWARE this could invalidate node references on resize!
    m_nodes.push_back(leftChild);
    m_nodes.push_back(rightChild);
    return true;
}

BVH::SplitData BVH::findBestSplit(u32 nodeIndex, const std::vector<AABB>& triangleAABBs) const {
    const Node& parentNode = m_nodes[nodeIndex];
    f32 parentCost = parentNode.triangleCount * AABBSurfaceArea(parentNode.aabb);  // Surface Area Heuristic

    f32 bestCost = parentCost;
    BVH::SplitData bestSplitData = {false, 0, 0.0f, AABB::empty(), AABB::empty()};

    for (u32 splitAxis = 0; splitAxis < 3; splitAxis++) {
        // Calculate real extents of the split axis from centers
        f32 axisStart = parentNode.aabb.min[splitAxis];
        f32 axisEnd = parentNode.aabb.max[splitAxis];
        for (u32 i = parentNode.triangleIndex; i < parentNode.triangleIndex + parentNode.triangleCount; i++) {
            axisStart = std::min(axisStart, triangleAABBs[i].center()[splitAxis]);
            axisEnd = std::max(axisEnd, triangleAABBs[i].center()[splitAxis]);
        }

        // Initialize bins
        f32 testInterval = (axisEnd - axisStart) / (m_perAxisSplitTests + 1);
        std::vector<std::pair<AABB, u32>> intervalBins(m_perAxisSplitTests + 1, {AABB::empty(), 0});

        // Bin triangles
        for (u32 i = parentNode.triangleIndex; i < parentNode.triangleIndex + parentNode.triangleCount; i++) {
            u32 binIndex = floor((triangleAABBs[i].center()[splitAxis] - axisStart) / testInterval);
            binIndex = glm::clamp(binIndex, 0U, m_perAxisSplitTests);
            intervalBins[binIndex].first = intervalBins[binIndex].first.boundingUnion(triangleAABBs[i]);
            intervalBins[binIndex].second++;
        }

        for (u32 splitNum = 0; splitNum < m_perAxisSplitTests; splitNum++) {
            SplitData split = {
                .shouldSplit = true,
                .splitAxis = splitAxis,
                .splitPoint = axisStart + testInterval * (splitNum + 1),
                .leftAABB = AABB::empty(),
                .rightAABB = AABB::empty(),
            };
            u32 leftCount = 0;

            // Sum up left and right bins
            for (u32 i = 0; i <= splitNum; i++) {
                split.leftAABB = split.leftAABB.boundingUnion(intervalBins[i].first);
                leftCount += intervalBins[i].second;
            }
            for (u32 i = splitNum + 1; i < intervalBins.size(); i++)
                split.rightAABB = split.rightAABB.boundingUnion(intervalBins[i].first);

            if (leftCount == 0 || leftCount == parentNode.triangleCount)
                continue;

            // Calculate SAH cost
            f32 leftCost = leftCount * AABBSurfaceArea(split.leftAABB);
            f32 rightCost = (parentNode.triangleCount - leftCount) * AABBSurfaceArea(split.rightAABB);
            f32 cost = leftCost + rightCost;

            if (cost < bestCost) {
                bestCost = cost;
                bestSplitData = split;
            }
        }
    }

    return bestSplitData;
}
