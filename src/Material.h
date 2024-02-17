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
        auto reflected = reflect(glm::normalize(ray.direction()), hit.normal);
        reflected += m_fuzziness * randomUnitVec3();
        scattered = Ray(hit.point, reflected);
        attenuation = m_albedo;
        return glm::dot(reflected, hit.normal) > 0;
    }
};

class DielectricMaterial : public Material {
public:
    float m_ir;

    DielectricMaterial(float ir) : m_ir(ir) {}

    virtual bool scatter(const Ray& ray, const HitRecord& hit, vec3& attenuation, Ray& scattered) const override {
        auto normalizedDirection = glm::normalize(ray.direction());
        auto refractionRatio = hit.frontFace ? 1.0f / m_ir : m_ir;

        auto cosTheta = std::min(glm::dot(-normalizedDirection, hit.normal), 1.0f);
        auto sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);

        vec3 outgoingDirection;

        bool totalInternalReflection = refractionRatio * sinTheta > 1.0f;
        if (totalInternalReflection || reflectance(cosTheta, refractionRatio) > randomFloat())
            outgoingDirection = reflect(normalizedDirection, hit.normal);
        else
            outgoingDirection = ::refract(normalizedDirection, hit.normal, refractionRatio);

        attenuation = vec3(1);
        scattered = Ray(hit.point, outgoingDirection);
        return true;
    }
};
