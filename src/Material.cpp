#include "Material.h"

SCATTER_FUNCTION(lambertianScatter) {
    // Alpha-clip
    f32 alpha = material.alphaTexture ? material.alphaTexture->sampleInterpolated(hit.uv) : 1.0;
    if (alpha < 0.5f) {
        hit.hit = false;
        return {};
    }

    // Normal mapping
    if (material.normalTexture) {
        mat3 tbn = mat3(hit.tangent, hit.bitangent, hit.normal);
        vec3 normalMapSample = material.normalTexture->sampleInterpolated(hit.uv) * 2.0f - 1.0f;
        hit.normal = glm::normalize(tbn * normalMapSample);
    }

    // Lambert
    auto scatterDirection = hit.normal + randomUnitVec<3>();

    if (glm::any(glm::abs(scatterDirection) < vec3(1e-8f)))  // Near zero direction fix
        scatterDirection = hit.normal;

    auto albedo = material.albedoTexture ? material.albedoTexture->sampleInterpolated(hit.uv) : material.albedo;
    auto emission = material.emissionTexture ? material.emissionTexture->sampleInterpolated(hit.uv) : material.emission;

    return {
        .scatterDirection = glm::normalize(scatterDirection),
        .albedo = albedo,
        .emission = emission * material.emissionIntensity,
    };
}

SCATTER_FUNCTION(metallicScatter) {
    // Alpha-clip
    f32 alpha = material.alphaTexture ? material.alphaTexture->sampleInterpolated(hit.uv) : 1.0;
    if (alpha < 0.5f) {
        hit.hit = false;
        return {};
    }

    // Normal mapping
    if (material.normalTexture) {
        mat3 tbn = mat3(hit.tangent, hit.bitangent, hit.normal);
        vec3 normalMapSample = material.normalTexture->sampleInterpolated(hit.uv) * 2.0f - 1.0f;
        hit.normal = glm::normalize(tbn * normalMapSample);
    }

    // Metallic
    auto reflected = reflect(ray.direction, hit.normal);
    reflected += material.fuzziness * randomUnitVec<3>();

    auto albedo = material.albedoTexture ? material.albedoTexture->sampleInterpolated(hit.uv) : material.albedo;
    auto emission = material.emissionTexture ? material.emissionTexture->sampleInterpolated(hit.uv) : material.emission;

    return {
        .didScatter = glm::dot(reflected, hit.normal) > 0,
        .scatterDirection = glm::normalize(reflected),
        .albedo = albedo,
        .emission = emission * material.emissionIntensity,
    };
}

SCATTER_FUNCTION(dielectricScatter) {
    // Normal mapping
    if (material.normalTexture) {
        mat3 tbn = mat3(hit.tangent, hit.bitangent, hit.normal);
        vec3 normalMapSample = material.normalTexture->sampleInterpolated(hit.uv) * 2.0f - 1.0f;
        hit.normal = glm::normalize(tbn * normalMapSample);
    }

    // Dielectric
    bool frontFaceHit = glm::dot(ray.direction, hit.normal) <= 0;
    auto refractionRatio = frontFaceHit ? 1.0f / material.ir : material.ir;
    vec3 outwardNormal = frontFaceHit ? hit.normal : -hit.normal;

    auto cosTheta = glm::dot(-ray.direction, outwardNormal);
    auto sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);

    vec3 scatterDirection;  // normalized

    bool totalInternalReflection = refractionRatio * sinTheta > 1.0f;
    if (totalInternalReflection || reflectance(cosTheta, refractionRatio) > random<f32>())
        scatterDirection = reflect(ray.direction, outwardNormal);
    else
        scatterDirection = ::refract(ray.direction, outwardNormal, refractionRatio);

    return {
        .isTransmission = true,
        .scatterDirection = scatterDirection,
        .albedo = vec3(1),
    };
}

SCATTER_FUNCTION(environmentScatter) {
    hit.normal = -ray.direction;

    vec3 emission;
    if (material.emissionTexture) {
        f32 u = atan2(ray.direction.z, ray.direction.x) / TWO_PI + 0.5f;
        f32 v = acos(ray.direction.y) / PI;
        emission = material.emissionTexture->sampleInterpolated({u, v});
    }
    else
        emission = material.emission;

    // emission =  glm::mix(vec3(1.0f), vec3(0.5f, 0.7f, 1.0f), 0.5f * (unitDirection.y + 1.0f)); // sky gradient

    return {
        .didScatter = false,
        .albedo = vec3(1),
        .emission = emission * material.emissionIntensity,
    };
}
