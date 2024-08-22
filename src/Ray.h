#pragma once

// TODO make min lower after addnig front face intersection settings
constexpr Interval<f32> RAY_INITIAL_INTERVAL = {0.001f, std::numeric_limits<f32>::infinity()};

struct Ray {
    vec3 origin;
    vec3 direction;  // normalized
    vec3 invDirection;
    Interval<f32> tInterval = RAY_INITIAL_INTERVAL;

#ifdef BVH_TEST
    u32 aabbTestCount = 0;
    u32 triangleTestCount = 0;
#endif

    explicit Ray() = default;

    Ray(const vec3& origin, const vec3& normalizedDirection, Interval<f32> tInterval = RAY_INITIAL_INTERVAL)
        : origin(origin), direction(normalizedDirection), invDirection(1.0f / direction), tInterval(tInterval) {}

    inline vec3 at(f32 t) const {
        return origin + t * direction;
    }

    inline Ray createTransformedRay(const mat4& transform) const {
        vec3 transformedOrigin = vec3(transform * vec4(origin, 1.0f));
        vec3 transformedDirection = vec3(transform * vec4(direction, 0.0f));
        f32 transformedDirectionLength = glm::length(transformedDirection);

        auto transformedTInterval = tInterval;
        if (!std::isinf(tInterval.min))
            transformedTInterval.min = tInterval.min * transformedDirectionLength;
        if (!std::isinf(tInterval.max))
            transformedTInterval.max = tInterval.max * transformedDirectionLength;

        Ray transformedRay(transformedOrigin, transformedDirection / transformedDirectionLength, transformedTInterval);

#ifdef BVH_TEST
        transformedRay.aabbTestCount = aabbTestCount;
        transformedRay.triangleTestCount = triangleTestCount;
#endif

        return transformedRay;
    }

    inline void updateFromTransformedRay(const Ray& transformedRay, const mat4& inverseTransform) {
        vec3 tMaxVec = inverseTransform * vec4(transformedRay.tInterval.max * transformedRay.direction, 0);
        tInterval.max = glm::length(tMaxVec);

#ifdef BVH_TEST
        aabbTestCount = transformedRay.aabbTestCount;
        triangleTestCount = transformedRay.triangleTestCount;
#endif
    }
};
