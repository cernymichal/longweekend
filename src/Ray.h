#pragma once

struct Ray {
    vec3 origin;
    vec3 direction;
    vec3 invDirection;

    explicit Ray() = default;

    Ray(const vec3& origin, const vec3& direction)
        : origin(origin), direction(direction), invDirection(1.0f / direction) {}

    inline vec3 at(f32 t) const {
        return origin + t * direction;
    }
};
