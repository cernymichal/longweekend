#include "BVH.h"

HitRecord BVH::intersect(Ray& ray, bool intersectBackfacing) const {
    HitRecord hit;

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
        if (isnan(nodeIntersection.min) || !ray.tInterval.intersects(nodeIntersection))
            continue;

        if (node.faceCount != 0) {
            // Leaf node
            for (u32 i = node.faceIndex; i < node.faceIndex + node.faceCount; i++) {
                const Face& face = m_faces->at(i);

#ifdef BVH_TEST
                ray.faceTestCount++;
#endif

                auto [t, barycentric] = rayTriangleIntersectionCoordinates(ray.origin, ray.direction, face.vertices, intersectBackfacing);
                if (!isnan(t) && ray.tInterval.surrounds(t)) {
                    // TODO reject if triangle is almost perpendicular to ray

                    hit.hit = true;
                    hit.face = &face;
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
        bool leftNodeHit = !isnan(leftNodeIntersection.min) && ray.tInterval.intersects(leftNodeIntersection);

        const Node& rightNode = m_nodes[node.childIndex + 1];
        auto rightNodeIntersection = rayAABBintersection(ray.origin, ray.invDirection, rightNode.aabb);
        bool rightNodeHit = !isnan(rightNodeIntersection.min) && ray.tInterval.intersects(rightNodeIntersection);

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

void BVH::build(std::vector<Face>& faces) {
    m_faces = &faces;
    m_nodes.clear();
    // TODO reserve m_nodes

    // Calculate AABBs for each face
    std::vector<AABB> faceAABBs;
    faceAABBs.reserve(faces.size());
    for (const Face& face : faces) {
        AABB aabb = AABB::empty();
        for (u32 i = 0; i < 3; i++)
            aabb = aabb.extendTo(face.vertices[i]);
        faceAABBs.push_back(aabb);
    }

    m_nodes.push_back({
        .aabb = AABB::empty(),
        .faceCount = (u32)faces.size(),
        .faceIndex = 0,
    });

    std::queue<std::pair<u32, u32>> queue;
    queue.push({0, 1});

    while (!queue.empty()) {
        u32 currentNodeIndex = queue.front().first;
        Node& currentNode = m_nodes[currentNodeIndex];
        u32 depth = queue.front().second;
        queue.pop();

        // Calculate node AABB
        currentNode.aabb = AABB::empty();
        for (u32 i = currentNode.faceIndex; i < currentNode.faceIndex + currentNode.faceCount; i++)
            currentNode.aabb = currentNode.aabb.boundingUnion(faceAABBs[i]);

        // Dont split if we have too few faces or reached max depth
        if (currentNode.faceCount <= BVH_MAX_FACES_PER_LEAF || depth >= BVH_MAX_DEPTH)
            continue;

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

        // Split faces along longest axis
        f32 splitPoint = currentNode.aabb.min[longestAxis] + axisLength / 2;
        u32 j = currentNode.faceIndex + currentNode.faceCount - 1;
        for (u32 i = currentNode.faceIndex; i < j;) {
            if (faceAABBs[i].max[longestAxis] < splitPoint)
                i++;
            else {
                std::swap(faces[i], faces[j]);
                std::swap(faceAABBs[i], faceAABBs[j]);
                j--;
            }
        }

        // Split node
        Node leftChild = {
            .faceCount = j - currentNode.faceIndex,
            .faceIndex = currentNode.faceIndex,
        };

        Node rightChild = {
            .faceCount = currentNode.faceCount - leftChild.faceCount,
            .faceIndex = j,
        };

        if (leftChild.faceCount == 0 || rightChild.faceCount == 0)
            continue;  // TODO handle this case, try another axis?

        currentNode.faceCount = 0;
        currentNode.childIndex = (u32)m_nodes.size();

        queue.push({currentNode.childIndex, depth + 1});
        queue.push({currentNode.childIndex + 1, depth + 1});

        // BEWARE this could invalidate node references on resize!
        m_nodes.push_back(leftChild);
        m_nodes.push_back(rightChild);
    }

    m_nodes.shrink_to_fit();
}
