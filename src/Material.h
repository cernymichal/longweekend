#pragma once

#include "Hittables/IHittable.h"
#include "Ray.h"

class Material {
public:
    std::string m_name = "Unnamed";

    struct ScatterOutput {
        bool didScatter = true;
        vec3 scatterDirection;
        vec3 attenuation = vec3(1);
        vec3 emission = vec3(0);
    };

    virtual ~Material() = default;

    virtual ScatterOutput scatter(const Ray& ray, const HitRecord& hit) const = 0;
};

class LambertianMaterial : public Material {
public:
    vec3 m_albedo = vec3(0.8f);
    vec3 m_emission = vec3(0);
    f32 m_emissionIntensity = 0;

    LambertianMaterial() = default;

    LambertianMaterial(const vec3& albedo, const vec3& emission = vec3(0), f32 emissionIntensity = 0)
        : m_albedo(albedo), m_emission(emission), m_emissionIntensity(emissionIntensity) {}

    virtual ScatterOutput scatter(const Ray& ray, const HitRecord& hit) const override {
        auto scatterDirection = hit.normal + randomUnitVec<3>();

        // near zero direction fix
        if (glm::any(glm::abs(scatterDirection) < vec3(1e-8f)))
            scatterDirection = hit.normal;

        return {
            .scatterDirection = scatterDirection,
            .attenuation = m_albedo,
            .emission = m_emission * m_emissionIntensity};
    }
};

class MetalMaterial : public Material {
public:
    vec3 m_albedo = vec3(0);
    f32 m_fuzziness = 0;

    MetalMaterial() = default;

    MetalMaterial(const vec3& albedo, f32 fuzziness) : m_albedo(albedo), m_fuzziness(fuzziness) {}

    virtual ScatterOutput scatter(const Ray& ray, const HitRecord& hit) const override {
        auto reflected = reflect(glm::normalize(ray.direction), hit.normal);
        reflected += m_fuzziness * randomUnitVec<3>();

        return {
            .didScatter = glm::dot(reflected, hit.normal) > 0,
            .scatterDirection = reflected,
            .attenuation = m_albedo};
    }
};

class DielectricMaterial : public Material {
public:
    f32 m_ir = 1;

    DielectricMaterial() = default;

    DielectricMaterial(f32 ir) : m_ir(ir) {}

    virtual ScatterOutput scatter(const Ray& ray, const HitRecord& hit) const override {
        auto normalizedDirection = glm::normalize(ray.direction);
        bool frontFaceHit = glm::dot(normalizedDirection, hit.normal) <= 0;
        auto refractionRatio = frontFaceHit ? 1.0f / m_ir : m_ir;
        vec3 outwardNormal = frontFaceHit ? hit.normal : -hit.normal;

        auto cosTheta = std::min(glm::dot(-normalizedDirection, outwardNormal), 1.0f);
        auto sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);

        vec3 scatterDirection;

        bool totalInternalReflection = refractionRatio * sinTheta > 1.0f;
        if (totalInternalReflection || reflectance(cosTheta, refractionRatio) > random<f32>())
            scatterDirection = reflect(normalizedDirection, outwardNormal);
        else
            scatterDirection = ::refract(normalizedDirection, outwardNormal, refractionRatio);

        return {
            .scatterDirection = scatterDirection};
    }
};
