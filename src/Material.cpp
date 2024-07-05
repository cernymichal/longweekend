#include "Material.h"

SCATTER_FUNCTION(lambertianScatter) {
    auto scatterDirection = hit.normal + randomUnitVec<3>();

    // near zero direction fix
    if (glm::any(glm::abs(scatterDirection) < vec3(1e-8f)))
        scatterDirection = hit.normal;

    auto albedo = material.albedoTexture ? material.albedoTexture->sampleInterpolated(hit.uv) : material.albedo;
    auto emission = material.emissionTexture ? material.emissionTexture->sampleInterpolated(hit.uv) : material.emission;

    // TODO normal mapping

    return {
        .scatterDirection = scatterDirection,
        .albedo = albedo,
        .emission = emission * material.emissionIntensity,
    };
}

SCATTER_FUNCTION(metallicScatter) {
    auto reflected = reflect(glm::normalize(ray.direction), hit.normal);
    reflected += material.fuzziness * randomUnitVec<3>();

    auto albedo = material.albedoTexture ? material.albedoTexture->sampleInterpolated(hit.uv) : material.albedo;
    auto emission = material.emissionTexture ? material.emissionTexture->sampleInterpolated(hit.uv) : material.emission;

    return {
        .didScatter = glm::dot(reflected, hit.normal) > 0,
        .scatterDirection = reflected,
        .albedo = albedo,
        .emission = emission * material.emissionIntensity,
    };
}

SCATTER_FUNCTION(dielectricScatter) {
    auto normalizedDirection = glm::normalize(ray.direction);
    bool frontFaceHit = glm::dot(normalizedDirection, hit.normal) <= 0;
    auto refractionRatio = frontFaceHit ? 1.0f / material.ir : material.ir;
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
        .scatterDirection = scatterDirection,
        .albedo = vec3(1),
    };
}
