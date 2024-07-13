#pragma once

#include "Hittables/IHittable.h"
#include "Material.h"
#include "Texture.h"

enum class OutputChannel : u32 {
    Color = 1 << 0,
    Depth = 1 << 1,
    Normal = 1 << 2,
    Albedo = 1 << 3,
    Emission = 1 << 4,
#ifdef BVH_TEST
    AABBTestCount = 1 << 5,
    FaceTestCount = 1 << 6,
#endif
};

struct RenderOuput {
    Texture<vec3> color;
    Texture<f32> depth;
    Texture<vec3> normal;
    Texture<vec3> albedo;
    Texture<vec3> emission;
#ifdef BVH_TEST
    Texture<f32> aabbTestCount;
    Texture<f32> faceTestCount;
#endif
};

struct AuxillaryRayOutput {
    f32 depth = INFINITY;
    vec3 normal = vec3(0);
    vec3 albedo = vec3(0);
    vec3 emission = vec3(0);
#ifdef BVH_TEST
    u32 aabbTestCount = 0;
    u32 faceTestCount = 0;
#endif
};

template <typename T>
inline T runningAverage(const T& previousValue, const T& currentValue, u32 currentSampleCount) {
    if (currentSampleCount == 1)
        return currentValue;

    return (previousValue * (f32)(currentSampleCount - 1) + currentValue) / f32(currentSampleCount);
}

class Camera {
public:
    glm::uvec2 m_imageSize = glm::uvec2(256, 256);
    u32 m_samples = 32;
    u32 m_auxillarySamples = 8;
    u32 m_maxBounces = 10;

    f32 m_fov = 90.0f;
    vec3 m_position = vec3(0);
    vec3 m_lookAt = VEC_FORWARD;  // TODO add transform to camera
    vec3 m_up = VEC_UP;

    f32 m_defocusAngle = 0.0f;
    f32 m_focusDistance = 10.0f;

    u32 m_outputChannels = (u32)OutputChannel::Color;

    Ref<Material> m_environmentMaterial = makeRef<Material>(Material{
        .name = "Environment",
        .emission = vec3(0.05f),
        .emissionIntensity = 1.0f,
        .scatterFunction = environmentScatter,
    });

    RenderOuput render(IHittable& world, std::function<void(const RenderOuput&, u32)> colorSampleCallback, std::function<void(const RenderOuput&, u32)> auxillarySampleCallback) {
        initialize();
        world.frameBegin();

        RenderOuput output{
            .color = Texture<vec3>(m_outputChannels & (u32)OutputChannel::Color ? m_imageSize : uvec2(0)),
            .depth = Texture<f32>(m_outputChannels & (u32)OutputChannel::Depth ? m_imageSize : uvec2(0)),
            .normal = Texture<vec3>(m_outputChannels & (u32)OutputChannel::Normal ? m_imageSize : uvec2(0)),
            .albedo = Texture<vec3>(m_outputChannels & (u32)OutputChannel::Albedo ? m_imageSize : uvec2(0)),
            .emission = Texture<vec3>(m_outputChannels & (u32)OutputChannel::Emission ? m_imageSize : uvec2(0)),
#ifdef BVH_TEST
            .aabbTestCount = Texture<f32>(m_outputChannels & (u32)OutputChannel::AABBTestCount ? m_imageSize : uvec2(0)),
            .faceTestCount = Texture<f32>(m_outputChannels & (u32)OutputChannel::FaceTestCount ? m_imageSize : uvec2(0)),
#endif
        };

        renderAuxillaryChannels(output, world, auxillarySampleCallback);  // TODO merge
        renderColorChannel(output, world, colorSampleCallback);

        return output;
    }

private:
    vec2 m_viewportSize = vec2(1);
    vec3 m_pixelDeltaU = vec3(0);
    vec3 m_pixelDeltaV = vec3(0);
    vec3 m_pixelGridOrigin = vec3(0);
    vec3 m_defocusDiskU = vec3(0);
    vec3 m_defocusDiskV = vec3(0);

    void initialize() {
        auto aspectRatio = static_cast<f32>(m_imageSize.x) / m_imageSize.y;
        m_viewportSize = vec2(aspectRatio, 1) * 2.0f * m_focusDistance * glm::tan(glm::radians(m_fov / 2.0f));

        vec3 u, v, w;
        w = glm::normalize(m_position - m_lookAt);
        u = glm::normalize(glm::cross(m_up, w));
        v = glm::cross(w, u);

        vec3 viewportU, viewportV;
        viewportU = u * m_viewportSize.x;
        viewportV = -v * m_viewportSize.y;
        m_pixelDeltaU = viewportU / static_cast<f32>(m_imageSize.x);
        m_pixelDeltaV = viewportV / static_cast<f32>(m_imageSize.y);
        m_pixelGridOrigin = m_position - m_focusDistance * w - viewportU / 2.0f - viewportV / 2.0f + m_pixelDeltaU / 2.0f + m_pixelDeltaV / 2.0f;

        auto defocusRadius = m_focusDistance * tan(glm::radians(m_defocusAngle / 2));
        m_defocusDiskU = defocusRadius * u;
        m_defocusDiskV = defocusRadius * v;
    }

    void renderAuxillaryChannels(RenderOuput& output, IHittable& world, std::function<void(const RenderOuput&, u32)> auxillarySampleCallback) {
        if (!(m_outputChannels & ~(u32)OutputChannel::Color))
            return;

        // Accumulate auxillary samples
        for (u32 sample = 1; sample <= m_auxillarySamples; sample++) {
            // TODO uniform sampling
            vec2 jitter = randomVec<2>() - 0.5f;
            vec3 jitteredSamplePoint = jitter.x * m_pixelDeltaU + jitter.y * m_pixelDeltaU;

            // Sample the whole frame
            NODEBUG_ONLY(_Pragma("omp parallel for"))
            for (u32 y = 0; y < m_imageSize.y; y++) {
                for (u32 x = 0; x < m_imageSize.x; x++) {
                    vec3 pixelCenter = m_pixelGridOrigin + static_cast<f32>(x) * m_pixelDeltaU + static_cast<f32>(y) * m_pixelDeltaV;
                    vec3 pixelSamplePoint = pixelCenter + jitteredSamplePoint;
                    vec3 rayOrigin = m_defocusAngle > 0 ? randomInDefocusDisk() : m_position;

                    AuxillaryRayOutput raySample = sampleRayAuxillary(Ray(rayOrigin, pixelSamplePoint - rayOrigin), world);

                    if (m_outputChannels & (u32)OutputChannel::Depth)
                        output.depth[uvec2(x, y)] = runningAverage(output.depth[uvec2(x, y)], raySample.depth, sample);
                    if (m_outputChannels & (u32)OutputChannel::Normal)
                        output.normal[uvec2(x, y)] = runningAverage(output.normal[uvec2(x, y)], raySample.normal, sample);
                    if (m_outputChannels & (u32)OutputChannel::Albedo)
                        output.albedo[uvec2(x, y)] = runningAverage(output.albedo[uvec2(x, y)], raySample.albedo, sample);
                    if (m_outputChannels & (u32)OutputChannel::Emission)
                        output.emission[uvec2(x, y)] = runningAverage(output.emission[uvec2(x, y)], raySample.emission, sample);
#ifdef BVH_TEST
                    if (m_outputChannels & (u32)OutputChannel::AABBTestCount)
                        output.aabbTestCount[uvec2(x, y)] = runningAverage(output.aabbTestCount[uvec2(x, y)], (f32)raySample.aabbTestCount, sample);
                    if (m_outputChannels & (u32)OutputChannel::FaceTestCount)
                        output.faceTestCount[uvec2(x, y)] = runningAverage(output.faceTestCount[uvec2(x, y)], (f32)raySample.faceTestCount, sample);
#endif
                }
            }

            auxillarySampleCallback(output, sample);
        }
    }

    void renderColorChannel(RenderOuput& output, IHittable& world, std::function<void(const RenderOuput&, u32)> colorSampleCallback) {
        if (!(m_outputChannels & (u32)OutputChannel::Color))
            return;

        // Accumulate color samples
        for (u32 sample = 1; sample <= m_samples; sample++) {
            vec2 jitter = randomVec<2>() - 0.5f;
            vec3 jitteredSamplePoint = jitter.x * m_pixelDeltaU + jitter.y * m_pixelDeltaU;

            // Sample the whole frame
            NODEBUG_ONLY(_Pragma("omp parallel for"))
            for (u32 y = 0; y < m_imageSize.y; y++) {
                for (u32 x = 0; x < m_imageSize.x; x++) {
                    vec3 pixelCenter = m_pixelGridOrigin + static_cast<f32>(x) * m_pixelDeltaU + static_cast<f32>(y) * m_pixelDeltaV;
                    vec3 pixelSamplePoint = pixelCenter + jitteredSamplePoint;
                    vec3 rayOrigin = m_defocusAngle > 0 ? randomInDefocusDisk() : m_position;

                    vec3 raySample = sampleRayColor(Ray(rayOrigin, pixelSamplePoint - rayOrigin), world);

                    output.color[uvec2(x, y)] = runningAverage(output.color[uvec2(x, y)], raySample, sample);  // TODO check error vs division after finishing
                }
            }

            colorSampleCallback(output, sample);
        }
    }

    vec3 sampleRayColor(Ray&& ray, const IHittable& world) const {
        vec3 attenuation = vec3(1);
        vec3 incomingLight = vec3(0);

        for (u32 i = 0; i <= m_maxBounces; i++) {
            auto [hit, scatterOutput] = sampleRay(ray, world);

            ray = Ray(hit.point, scatterOutput.scatterDirection);
            incomingLight += attenuation * scatterOutput.emission;
            attenuation *= scatterOutput.albedo;

            // if (glm::length2(attenuation) < 0.001f)
            //     break;  // Early termination for small values, might break high exposure

            if (!scatterOutput.didScatter)
                break;  // Absorbed
        }

        return incomingLight;
    }

    AuxillaryRayOutput sampleRayAuxillary(Ray&& ray, const IHittable& world) const {
        AuxillaryRayOutput output;

        for (u32 i = 0; i < m_maxBounces; i++) {
            auto [hit, scatterOutput] = sampleRay(ray, world);

            if (scatterOutput.isTransmission) {
                ray = Ray(hit.point, scatterOutput.scatterDirection);
                continue;
            }

            output.depth = std::isinf(ray.tInterval.max) ? 0.0f : 1.0f / (ray.tInterval.max * glm::length(ray.direction) + 1.0f);  // Reverse depth
            output.normal = hit.normal;                                                                                            // hit.normal * 0.5f + 0.5f;
            output.albedo = scatterOutput.albedo;
            output.emission = scatterOutput.emission;
#ifdef BVH_TEST
            output.aabbTestCount = ray.aabbTestCount;
            output.faceTestCount = ray.faceTestCount;
#endif
            return output;
        }

        return output;
    }

    std::pair<HitRecord, ScatterOutput> sampleRay(Ray& ray, const IHittable& world) const {
        while (true) {  // TODO add max
            HitRecord hit = world.hit(ray);

            if (!hit.hit) {
                hit.hit = true;
                hit.material = m_environmentMaterial;
            }

            hit.point = ray.at(ray.tInterval.max);

            auto material = hit.material.lock();
            ScatterOutput scatterOutput;
            if (material->scatterFunction)
                scatterOutput = material->scatterFunction(*material, ray, hit);
            else {
                LOG(std::format("Scatter function not set for material: {}", material->name));
                scatterOutput = {
                    .scatterDirection = hit.normal + randomUnitVec<3>()};
            }

            if (!hit.hit) {
                // Alpha masked hit
                // TODO move this into the intersection check for performance?
                ray = Ray(hit.point, ray.direction);
                continue;
            }

            return {hit, scatterOutput};
        }
    }

    vec3 randomInDefocusDisk() const {
        auto r = randomVec2InUnitDisk();
        return m_position + m_defocusDiskU * r.x + m_defocusDiskV * r.y;
    }
};
