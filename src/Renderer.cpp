#include "Renderer.h"

template <typename T>
inline T runningAverage(const T& previousValue, const T& currentValue, u32 currentSampleCount) {
    if (currentSampleCount == 1)
        return currentValue;

    // Check for NaNs
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(currentValue))
            return previousValue;
        if (std::isnan(previousValue))
            return currentValue;
    }
    else if constexpr (std::is_same_v<T, vec3>) {
        if (glm::any(glm::isnan(currentValue)))
            return previousValue;
        if (glm::any(glm::isnan(previousValue)))
            return currentValue;
    }
    else
        static_assert(false);

    return (previousValue * (f32)(currentSampleCount - 1) + currentValue) / f32(currentSampleCount);
}

Renderer::Output Renderer::renderFrame(Ref<World> world, Ref<Camera> camera) {
    m_world = world;
    m_camera = camera;

    m_camera->initialize(m_imageSize);
    m_world->hierarchy.frameBegin();

    Output output{
        .color = Texture<vec3>(m_outputChannels & (u32)OutputChannel::Color ? m_imageSize : uvec2(0)),
        .depth = Texture<f32>(m_outputChannels & (u32)OutputChannel::Depth ? m_imageSize : uvec2(0)),
        .normal = Texture<vec3>(m_outputChannels & (u32)OutputChannel::Normal ? m_imageSize : uvec2(0)),
        .albedo = Texture<vec3>(m_outputChannels & (u32)OutputChannel::Albedo ? m_imageSize : uvec2(0)),
        .emission = Texture<vec3>(m_outputChannels & (u32)OutputChannel::Emission ? m_imageSize : uvec2(0)),
#ifdef BVH_TEST
        .aabbTestCount = Texture<f32>(m_outputChannels & (u32)OutputChannel::AABBTestCount ? m_imageSize : uvec2(0)),
        .triangleTestCount = Texture<f32>(m_outputChannels & (u32)OutputChannel::TriangleTestCount ? m_imageSize : uvec2(0)),
#endif
    };

    if (!m_outputChannels)
        return output;

    // Accumulate samples
    for (u32 sampleNum = 1; sampleNum <= m_samples; sampleNum++) {
        sampleFrame(output, sampleNum);
        m_sampleCallback(output, sampleNum);
    }

    return output;
}

void Renderer::sampleFrame(Output& output, u32 sampleNum) const {
    NODEBUG_ONLY(_Pragma("omp parallel for"))
    for (u32 y = 0; y < m_imageSize.y; y++) {
        uvec2 pixel = uvec2(0, y);
        for (; pixel.x < m_imageSize.x; pixel.x++) {
            vec2 pixelSamplePoint = randomVec<2>() - 0.5f;
            Ray ray = m_camera->createRay(pixel, pixelSamplePoint);
            PathSample sceneSample = samplePath(std::move(ray));

            // TODO check error vs division after finishing
            if (m_outputChannels & (u32)OutputChannel::Color)
                output.color[pixel] = runningAverage(output.color[pixel], sceneSample.color, sampleNum);
            if (m_outputChannels & (u32)OutputChannel::Depth)
                output.depth[pixel] = runningAverage(output.depth[pixel], sceneSample.depth, sampleNum);
            if (m_outputChannels & (u32)OutputChannel::Normal)
                output.normal[pixel] = runningAverage(output.normal[pixel], sceneSample.normal, sampleNum);
            if (m_outputChannels & (u32)OutputChannel::Albedo)
                output.albedo[pixel] = runningAverage(output.albedo[pixel], sceneSample.albedo, sampleNum);
            if (m_outputChannels & (u32)OutputChannel::Emission)
                output.emission[pixel] = runningAverage(output.emission[pixel], sceneSample.emission, sampleNum);
#ifdef BVH_TEST
            if (m_outputChannels & (u32)OutputChannel::AABBTestCount)
                output.aabbTestCount[pixel] = runningAverage(output.aabbTestCount[pixel], (f32)sceneSample.aabbTestCount, sampleNum);
            if (m_outputChannels & (u32)OutputChannel::TriangleTestCount)
                output.triangleTestCount[pixel] = runningAverage(output.triangleTestCount[pixel], (f32)sceneSample.triangleTestCount, sampleNum);
#endif
        }
    }
}

Renderer::PathSample Renderer::samplePath(Ray&& ray) const {
    PathSample output;

    vec3 attenuation = vec3(1);
    vec3 incomingLight = vec3(0);
    bool sampledNonDeltaBounce = false;

    for (u32 bounceNum = 0; bounceNum <= m_maxBounces; bounceNum++) {
        auto [hit, scatterOutput] = sampleRay(ray);

        incomingLight += attenuation * scatterOutput.emission;
        attenuation *= scatterOutput.albedo;

        if (bounceNum == 0) {
            output.depth = std::isinf(ray.tInterval.max) ? 0.0f : 1.0f / (ray.tInterval.max * glm::length(ray.direction) + 1.0f);  // Reverse depth
#ifdef BVH_TEST
            output.aabbTestCount = ray.aabbTestCount;
            output.triangleTestCount = ray.triangleTestCount;
#endif
        }

        if (!sampledNonDeltaBounce && !scatterOutput.isTransmission) {
            // Only sample for first non-delta bounce
            sampledNonDeltaBounce = true;

            output.normal = hit.normal;  // hit.normal * 0.5f + 0.5f;
            output.albedo = scatterOutput.albedo;
            output.emission = scatterOutput.emission;
        }

        // if (glm::length2(attenuation) < 0.001f)
        //     break;  // Early termination for small values, might break high exposure

        if (!scatterOutput.didScatter)
            break;  // Absorbed

        ray = Ray(hit.point, scatterOutput.scatterDirection);  // Bounce ray
    }

    output.color = incomingLight;

    return output;
}

std::pair<HitRecord, ScatterOutput> Renderer::sampleRay(Ray& ray) const {
    while (true) {  // TODO add max
        HitRecord hit = m_world->hierarchy.hit(ray);

        if (!hit.hit) {
            hit.hit = true;
            hit.material = m_world->environmentMaterial;
        }

        hit.point = ray.at(ray.tInterval.max);

        ScatterOutput scatterOutput;
        if (hit.material->scatterFunction)
            scatterOutput = hit.material->scatterFunction(*hit.material, ray, hit);
        else {
            LOG(std::format("Scatter function not set for material: {}", hit.material->name));
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
