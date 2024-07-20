#pragma once

// TODO make min lower after addnig front face intersection settings
constexpr Interval<f32> RAY_INITIAL_INTERVAL = {0.001f, std::numeric_limits<f32>::infinity()};

struct Ray {
    vec3 origin;
    vec3 direction;  // Doesn't need to be normalized
    vec3 invDirection;
    Interval<f32> tInterval = RAY_INITIAL_INTERVAL;

#ifdef BVH_TEST
    u32 aabbTestCount = 0;
    u32 triangleTestCount = 0;
#endif

    explicit Ray() = default;

    Ray(const vec3& origin, const vec3& direction, Interval<f32> tInterval = RAY_INITIAL_INTERVAL)
        : origin(origin), direction(direction), invDirection(1.0f / direction), tInterval(tInterval) {}

    inline vec3 at(f32 t) const {
        return origin + t * direction;
    }

    inline Ray createTransformedRay(const mat4& transform) const {
        auto transformedOrigin = vec3(transform * vec4(origin, 1.0f));
        auto transformedDirection = vec3(transform * vec4(direction, 0.0f));

        auto transformedTInterval = tInterval;
        auto transformedDirectionLength = glm::length(transformedDirection);
        if (!std::isinf(tInterval.min)) {
            vec3 tMinVec = transform * vec4(tInterval.min * direction, 0);
            transformedTInterval.min = glm::length(tMinVec) / transformedDirectionLength;
        }
        if (!std::isinf(tInterval.max)) {
            vec3 tMaxVec = transform * vec4(tInterval.max * direction, 0);
            transformedTInterval.max = glm::length(tMaxVec) / transformedDirectionLength;
        }

        Ray transformedRay(transformedOrigin, transformedDirection, transformedTInterval);

#ifdef BVH_TEST
        transformedRay.aabbTestCount = aabbTestCount;
        transformedRay.triangleTestCount = triangleTestCount;
#endif

        return transformedRay;
    }

    inline void updateFromTransformedRay(const Ray& transformedRay, const mat4& inverseTransform) {
        vec3 tMaxVec = inverseTransform * vec4(transformedRay.tInterval.max * transformedRay.direction, 0);
        tInterval.max = glm::length(tMaxVec) / glm::length(direction);

#ifdef BVH_TEST
        aabbTestCount = transformedRay.aabbTestCount;
        triangleTestCount = transformedRay.triangleTestCount;
#endif
    }
};
