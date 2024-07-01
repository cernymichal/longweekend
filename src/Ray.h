#pragma once

struct Ray {
    vec3 origin;
    vec3 direction; // does not need to be normalized
    vec3 invDirection;

    explicit Ray() = default;

    Ray(const vec3& origin, const vec3& direction)
        : origin(origin), direction(direction), invDirection(1.0f / direction) {}

    inline vec3 at(f32 t) const {
        return origin + t * direction;
    }
};

Ray operator*(const mat4& transform, const Ray& ray) {
	return Ray(vec3(transform * vec4(ray.origin, 1.0f)), vec3(transform * vec4(ray.direction, 0.0f)));
}
