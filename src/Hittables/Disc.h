#pragma once

#include "IHittable.h"
#include "Transform.h"

class Disc : public IHittable {
public:
    Transform m_transform;
    Ref<Material> m_material;
    vec2 m_size;

    explicit Disc(const Transform& transform, const Ref<Material>& material, const vec2& size = vec2(1))
        : m_transform(transform), m_material(material), m_size(size) {}

    HitRecord hit(Ray& ray) const override {
        float dot = glm::dot(m_normal, ray.direction);
        if (std::abs(dot) < 1e-6f)  // TODO front face based on material
            return HitRecord();

        f32 t = glm::dot(m_origin - ray.origin, m_normal) / dot;
        vec3 planeHitPoint = ray.at(t) - m_origin;
        vec2 uv = vec2(glm::dot(planeHitPoint, m_u), glm::dot(planeHitPoint, m_v)) * m_uvLength2Inv;

        HitRecord hit;
        hit.hit = ray.tInterval.surrounds(t) && glm::length(uv) <= 0.5f;

        if (hit.hit) {
            ray.tInterval.max = t;
            hit.normal = dot < 0.0f ? m_normal : -m_normal;  // TODO only front face based on material
            hit.uv = uv + vec2(0.5f);
            hit.material = m_material;
        }

        return hit;
    }

    void frameBegin() override {
        m_transform.updateMatrices();
        m_normal = m_transform.modelMatrix() * vec4(VEC_UP, 0.0f);
        m_origin = m_transform.position();
        m_u = m_transform.modelMatrix() * vec4(VEC_RIGHT * m_size.x, 0.0f);
        m_v = m_transform.modelMatrix() * vec4(VEC_FORWARD * m_size.y, 0.0f);
        m_uvLength2Inv = 1.0f / vec2(glm::length2(m_u), glm::length2(m_v));
    }

private:
    vec3 m_normal = VEC_UP;
    vec3 m_u = VEC_RIGHT;
    vec3 m_v = VEC_FORWARD;
    vec2 m_uvLength2Inv = vec2(1.0f);
    vec3 m_origin = vec3(0);
};
