#pragma once

#include "Camera.h"
#include "World.h"

class Renderer {
public:
    enum class OutputChannel : u32 {
        Color = BIT(0),
        Depth = BIT(1),
        Normal = BIT(2),
        Albedo = BIT(3),
        Emission = BIT(4),
#ifdef BVH_TEST
        AABBTestCount = BIT(5),
        TriangleTestCount = BIT(6),
#endif
    };

    struct Output {
        Texture<vec3> color;
        Texture<f32> depth;
        Texture<vec3> normal;
        Texture<vec3> albedo;
        Texture<vec3> emission;
#ifdef BVH_TEST
        Texture<f32> aabbTestCount;
        Texture<f32> triangleTestCount;
#endif
    };

    struct PathSample {
        vec3 color = vec3(0);
        f32 depth = INFINITY;
        vec3 normal = vec3(NAN);
        vec3 albedo = vec3(NAN);
        vec3 emission = vec3(NAN);
#ifdef BVH_TEST
        u32 aabbTestCount = NAN;
        u32 triangleTestCount = NAN;
#endif
    };

    glm::uvec2 m_imageSize = glm::uvec2(256, 256);
    u32 m_samples = 32;
    u32 m_maxBounces = 10;

    u32 m_outputChannels = (u32)OutputChannel::Color;

    std::function<void(const Output&, u32)> m_sampleCallback;

    Output renderFrame(Ref<World> world, Ref<Camera> camera);

private:
    Ref<World> m_world;
    Ref<Camera> m_camera;

    void sampleFrame(Output& output, u32 sampleNum) const;

    PathSample samplePath(Ray&& ray) const;

    std::pair<HitRecord, ScatterOutput> sampleRay(Ray& ray) const;
};
