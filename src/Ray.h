#pragma once

class Ray {
public:
    Ray() = default;

    Ray(const vec3& origin, const vec3& direction) : m_origin(origin), m_direction(direction) {}

    inline const vec3& origin() const { return m_origin; }

    inline const vec3& direction() const { return m_direction; }

    inline vec3 at(f32 t) const { return m_origin + t * m_direction; }

private:
    vec3 m_origin;
    vec3 m_direction;
};
