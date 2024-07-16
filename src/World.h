#pragma once

#include "Hittables/HittableGroup.h"
#include "Material.h"

struct World {
    HittableGroup hierarchy;
    Ref<Material> environmentMaterial = makeRef<Material>(Material{
        .name = "Environment",
        .emission = vec3(0.05f),
        .emissionIntensity = 1.0f,
        .scatterFunction = environmentScatter,
    });
};
