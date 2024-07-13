#pragma once

#include "Texture.h"

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
inline vec3 acesApproximation(vec3 color) {
    color *= 0.6f;
    const f32 a = 2.51f;
    const f32 b = 0.03f;
    const f32 c = 2.43f;
    const f32 d = 0.59f;
    const f32 e = 0.14f;
    return glm::clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0f, 1.0f);
}

inline u8vec3 hdrToSRGB(const vec3& color, f32 gamma = 2.2f) {
    vec3 output = acesApproximation(color);         // tone mapping
    output = glm::pow(output, vec3(1.0f / gamma));  // gamma correction
    output = glm::clamp(output, vec3(0), vec3(1));  // clamp to [0, 1]
    return u8vec3(output * 255.0f);                 // convert to 8-bit
}

inline Texture<u8vec3> hdrToSRGB(const Texture<vec3>& texture, f32 gamma = 2.2f) {
    Texture<u8vec3> output(texture.size());
    NODEBUG_ONLY(_Pragma("omp parallel for"))
    for (u32 y = 0; y < texture.size().y; y++) {
        for (u32 x = 0; x < texture.size().x; x++)
            output[uvec2(x, y)] = hdrToSRGB(texture[uvec2(x, y)], gamma);
    }

    return output;
}

Texture<vec3> denoiseTextureOIDN(const Texture<vec3>& color, const Texture<vec3>& albedo, const Texture<vec3>& normal, bool prefilterAuxillary = false);
