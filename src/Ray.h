#pragma once

class Ray {
public:
    Ray() = default;

    Ray(const vec3& origin, const vec3& direction) : m_origin(origin), m_direction(direction) {}

    const vec3& origin() const { return m_origin; }

    const vec3& direction() const { return m_direction; }

    vec3 at(float t) const { return m_origin + t * m_direction; }

private:
    vec3 m_origin;
    vec3 m_direction;
};
