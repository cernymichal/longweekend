#pragma once

#include "Hittables/IHittable.h"
#include "Ray.h"
#include "Texture.h"

struct ScatterOutput {
    bool didScatter = true;
    vec3 scatterDirection;
    vec3 albedo = DEBUG_COLOR;
    vec3 emission = vec3(0);
};

#define SCATTER_FUNCTION(name) ScatterOutput name(const Material& material, const Ray& ray, HitRecord& hit)

SCATTER_FUNCTION(lambertianScatter);

SCATTER_FUNCTION(metallicScatter);

SCATTER_FUNCTION(dielectricScatter);

SCATTER_FUNCTION(environmentScatter);

struct Material {
public:
    std::string name = "Unnamed";

    vec3 albedo = vec3(0.8f);
    Ref<Texture<vec3>> albedoTexture;
    vec3 emission = vec3(0);
    Ref<Texture<vec3>> emissionTexture;
    f32 emissionIntensity = 0;
    Ref<Texture<vec3>> normalTexture;

    f32 fuzziness = 0;
    f32 ir = 1;

    SCATTER_FUNCTION((*scatterFunction)) = lambertianScatter;
};
