#pragma once

#include "IHittable.h"
#include "Ray.h"

class Material {
public:
    virtual ~Material() = default;

    virtual bool scatter(const Ray& ray, const HitRecord& hit, vec3& attenuation, Ray& scattered) const = 0;
};

class LambertianMaterial : public Material {
public:
    vec3 m_albedo;

    LambertianMaterial(const vec3& albedo) : m_albedo(albedo) {}

    virtual bool scatter(const Ray& ray, const HitRecord& hit, vec3& attenuation, Ray& scattered) const override {
        auto scatterDirection = hit.normal + randomUnitVec3();

        // near zero direction fix
        if (glm::any(glm::lessThan(glm::abs(scatterDirection), vec3(1e-8f))))
            scatterDirection = hit.normal;

        scattered = Ray(hit.point, scatterDirection);
        attenuation = m_albedo;
        return true;
    }
};

class MetalMaterial : public Material {
public:
    vec3 m_albedo;
    float m_fuzziness;

    MetalMaterial(const vec3& albedo, float fuzziness) : m_albedo(albedo), m_fuzziness(fuzziness) {}

    virtual bool scatter(const Ray& ray, const HitRecord& hit, vec3& attenuation, Ray& scattered) const override {
        auto reflected = reflect(normalize(ray.direction()), hit.normal);
        reflected += m_fuzziness * randomUnitVec3();
        scattered = Ray(hit.point, reflected);
        attenuation = m_albedo;
        return glm::dot(reflected, hit.normal) > 0;
    }
};