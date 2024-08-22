#include "Hittables/InfinitePlane.h"
#include "Hittables/Rectangle.h"
#include "Hittables/Sphere.h"
#include "Hittables/TransformedInstance.h"
#include "IO/MeshIO.h"
#include "IO/TextureIO.h"
#include "Postprocessing.h"
#include "Renderer.h"

constexpr bool ENABLE_PREVIEW = true;
constexpr bool DENOISE_PREVIEW = true;
constexpr auto PROGRESS_VIEW_UPDATE_INTERVAL = std::chrono::seconds(5);
const std::filesystem::path OUTPUT_FOLDER = "output";

constexpr u32 DENOISE_CHANNELS = (u32)Renderer::OutputChannel::Color | (u32)Renderer::OutputChannel::Albedo | (u32)Renderer::OutputChannel::Normal;

std::pair<Ref<World>, Ref<Camera>> sphereScene() {
    auto world = makeRef<World>();
    auto camera = makeRef<Camera>();

    // camera
    camera->m_position = vec3(-2, .2, 1);
    camera->m_lookAt = vec3(-.15, .4, -1);
    camera->m_fov = 60.0f;
    camera->m_defocusAngle = 3.0f;
    camera->m_focusDistance = glm::length(camera->m_position - camera->m_lookAt);

    world->environmentMaterial->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto groundMaterial = makeRef<Material>();
    *groundMaterial = {
        .albedo = vec3(0.8, 0.8, 0.0),
    };

    auto centerMaterial = makeRef<Material>();
    *centerMaterial = {
        .albedo = vec3(0.1, 0.2, 0.5),
    };

    auto leftMaterial = makeRef<Material>();
    *leftMaterial = {
        .ir = 1.5f,
        .scatterFunction = dielectricScatter,
    };

    auto rightMaterial = makeRef<Material>();
    *rightMaterial = {
        .albedo = vec3(0.8, 0.6, 0.2),
        .fuzziness = 0.0f,
        .scatterFunction = metallicScatter,
    };

    world->hierarchy.add(makeRef<InfinitePlane>(vec3(0.0, -0.5, -1.0), VEC_UP, groundMaterial));
    world->hierarchy.add(makeRef<Sphere>(vec3(0.0, 0.0, -1), 0.5f, centerMaterial));
    world->hierarchy.add(makeRef<Sphere>(vec3(-1.0, 0.0, -1), 0.5f, leftMaterial));
    world->hierarchy.add(makeRef<Sphere>(vec3(-1.0, 0.0, -1), -0.4f, leftMaterial));
    world->hierarchy.add(makeRef<Sphere>(vec3(1.0, 0.0, -1), 0.5f, rightMaterial));

    return {world, camera};
}

std::pair<Ref<World>, Ref<Camera>> randomSphereScene() {
    auto world = makeRef<World>();
    auto camera = makeRef<Camera>();

    // camera
    camera->m_position = vec3(13, 2, 3);
    camera->m_lookAt = vec3(0, 0, 0);
    camera->m_fov = 20.0f;
    camera->m_defocusAngle = 0.6f;
    camera->m_focusDistance = 10.0f;

    world->environmentMaterial->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto groundMaterial = makeRef<Material>();
    *groundMaterial = {
        .albedo = vec3(0.5, 0.5, 0.5),
    };
    world->hierarchy.add(makeRef<InfinitePlane>(vec3(0, 0, 0), VEC_UP, groundMaterial));

    u32 sphereGridSize = 32;
    for (u32 i = 0; i < sphereGridSize * sphereGridSize; i++) {
        vec2 xz = (randomVec2Stratified(sphereGridSize, i) - vec2(0.7f, 0.5f)) * (f32)sphereGridSize;
        vec3 center = vec3(xz.x, 0.2, xz.y);
        if (glm::length(center - vec3(4, 0.2, 0)) < 1.2 || glm::length(center - vec3(0, 0.2, 0)) < 1.2 || glm::length(center - vec3(-4, 0.2, 0)) < 1.2)
            continue;

        auto sphereMaterial = makeRef<Material>();
        auto chooseMat = random<f64>();
        if (chooseMat < 0.7) {
            // diffuse
            *sphereMaterial = {
                .albedo = randomVec<3>() * randomVec<3>(),
            };
        }
        else if (chooseMat < 0.8) {
            // metal
            *sphereMaterial = {
                .albedo = randomVec<3>(vec3(0.5), vec3(1)),
                .fuzziness = random<f32>(0, 0.5),
                .scatterFunction = metallicScatter,
            };
        }
        else if (chooseMat < 0.9) {
            // emissive
            *sphereMaterial = {
                .albedo = vec3(0),
                .emission = randomVec<3>(),
                .emissionIntensity = random<f32>(10, 50),
            };
        }
        else {
            // glass
            *sphereMaterial = {
                .ir = 1.5f,
                .scatterFunction = dielectricScatter,
            };
        }

        world->hierarchy.add(makeRef<Sphere>(center, 0.2, sphereMaterial));
    }

    auto material1 = makeRef<Material>();
    *material1 = {
        .ir = 1.5f,
        .scatterFunction = dielectricScatter,
    };
    world->hierarchy.add(makeRef<Sphere>(vec3(0, 1, 0), 1.0, material1));

    auto material2 = makeRef<Material>();
    *material2 = {
        .albedo = vec3(0.4, 0.2, 0.1),
    };
    world->hierarchy.add(makeRef<Sphere>(vec3(-4, 1, 0), 1.0, material2));

    auto material3 = makeRef<Material>();
    *material3 = {
        .albedo = vec3(0.7, 0.6, 0.5),
        .fuzziness = 0.0f,
        .scatterFunction = metallicScatter,
    };
    world->hierarchy.add(makeRef<Sphere>(vec3(4, 1, 0), 1.0, material3));

    return {world, camera};
}

std::pair<Ref<World>, Ref<Camera>> teapotDragonScene() {
    auto world = makeRef<World>();
    auto camera = makeRef<Camera>();

    // camera
    camera->m_position = vec3(0, 0.1, 0.7);
    camera->m_lookAt = vec3(0, 0, 0);
    camera->m_fov = 48.0f;

    world->environmentMaterial->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto groundMaterial = makeRef<Material>();
    *groundMaterial = {
        .albedo = vec3(0.05f),
    };
    world->hierarchy.add(makeRef<Rectangle>(Transform(vec3(0.0f, -0.15f, 0.0f)), groundMaterial));

    auto teapotModel = makeRef<Model>(loadOBJ("resources/teapot.obj"));
    *teapotModel->m_mesh.materials[0] = {
        .ir = 1.5f,
        .scatterFunction = dielectricScatter,
    };
    auto teapot = makeRef<TransformedInstance<Model>>(teapotModel, Transform(vec3(-0.12, -0.1, 0.3), glm::radians(vec3(0, -20, 0)), vec3(1.5)));
    world->hierarchy.add(teapot);

    auto dragonModel = makeRef<Model>(loadOBJ("resources/dragon.obj"));
    *dragonModel->m_mesh.materials[0] = {
        .albedo = vec3(0.5, 0.6, 0.8),
    };
    auto dragon = makeRef<TransformedInstance<Model>>(dragonModel, Transform(vec3(0.1, 0.02, 0.0), glm::radians(vec3(0, 110, 0)), vec3(0.6)));
    world->hierarchy.add(dragon);

    auto sphere_material = makeRef<Material>();
    *sphere_material = {
        .albedo = vec3(0),
        .emission = vec3(1),
        .emissionIntensity = 10.0f,
    };
    auto sphere = makeRef<Sphere>(vec3(-0.1, 0.15, 0.1), 0.02, sphere_material);
    world->hierarchy.add(sphere);

    camera->m_defocusAngle = 0.6f;
    camera->m_focusDistance = glm::distance(camera->m_position, sphere->m_center) - 0.1f;

    return {world, camera};
}

std::pair<Ref<World>, Ref<Camera>> reimuScene() {
    auto world = makeRef<World>();
    auto camera = makeRef<Camera>();

    // camera
    camera->m_position = vec3(0, 0, 2);
    camera->m_lookAt = vec3(0, 0, 0);
    camera->m_fov = 48.0f;

    world->environmentMaterial->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto groundMaterial = makeRef<Material>();
    *groundMaterial = {
        .albedo = vec3(0.05f),
    };
    world->hierarchy.add(makeRef<Rectangle>(Transform(vec3(0.0f, -0.43f, 0.3f), glm::radians(vec3(0, -45, 0)), vec3(2.0f)), groundMaterial));

    auto reimuModel = makeRef<Model>(loadOBJ("resources/reimu/reimu.obj"));
    auto reimu = makeRef<TransformedInstance<Model>>(reimuModel, Transform(vec3(0.5, 0.15, 0.5), glm::radians(vec3(0, -90, 0)), vec3(1.0 / 20.0)));

    world->hierarchy.add(reimu);

    return {world, camera};
}

std::pair<Ref<World>, Ref<Camera>> sponzaScene() {
    auto world = makeRef<World>();
    auto camera = makeRef<Camera>();

    // camera
    camera->m_position = vec3(8, 1.5, 0);
    camera->m_lookAt = vec3(6, 1.7, 0);
    camera->m_fov = 50.0f;

    world->environmentMaterial->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto sponzaModel = makeRef<Model>(loadOBJ("resources/sponza/sponza.obj"));
    auto sponza = makeRef<TransformedInstance<Model>>(sponzaModel, Transform(vec3(0.0f), vec3(0.0f), vec3(1.0 / 100.0)));
    world->hierarchy.add(sponza);

    auto lightMaterial = makeRef<Material>();
    *lightMaterial = {
        .albedo = vec3(0),
        .emission = vec3(1, 0.92, 0.95),
        .emissionIntensity = 50.0f,
    };
    world->hierarchy.add(makeRef<Sphere>(vec3(-4, 2.5, 1), 0.3f, lightMaterial));
    world->hierarchy.add(makeRef<Sphere>(vec3(4, 0.5, -1.5), 0.3f, lightMaterial));

    return {world, camera};
}

std::pair<Ref<World>, Ref<Camera>> normalTestScene() {
    auto world = makeRef<World>();
    auto camera = makeRef<Camera>();

    // camera
    camera->m_position = vec3(0, 0, 2);
    camera->m_lookAt = vec3(0, 0, 0);
    camera->m_fov = 48.0f;

    world->environmentMaterial->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto cubeModel = makeRef<Model>(loadOBJ("resources/normal_test/normal_test.obj"));
    auto cube = makeRef<TransformedInstance<Model>>(cubeModel, Transform(vec3(0.0f), glm::radians(vec3(30, -30, 0)), vec3(1.0 / 2.0)));

    world->hierarchy.add(cube);

    return {world, camera};
}

void render() {
    // Setup renderer
    Renderer renderer;
    renderer.m_imageSize = uvec2(640, 480);
    renderer.m_samples = 128;
    renderer.m_maxBounces = 8;
    renderer.m_outputChannels = (u32)Renderer::OutputChannel::Color | (u32)Renderer::OutputChannel::Albedo | (u32)Renderer::OutputChannel::Normal;
    f32 gamma = 2.2f;

    bool canBeDenoised = (renderer.m_outputChannels & DENOISE_CHANNELS) == DENOISE_CHANNELS;
    if (DENOISE_PREVIEW && !canBeDenoised)
        LOG("Denoising preview is disabled because the output channels do not include color, albedo, or normal");

    auto previewNextUpdate = std::chrono::high_resolution_clock::now();
    renderer.m_sampleCallback = [&](const Renderer::Output& output, u32 sample) {
        LOG(std::format("{}/{} samples done ({:.1f}%)", sample, renderer.m_samples, sample * 100.0f / renderer.m_samples));

        if (ENABLE_PREVIEW && output.color.size() != uvec2(0)) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            if (previewNextUpdate <= currentTime || sample == renderer.m_samples) {
                auto preview = DENOISE_PREVIEW && canBeDenoised ? denoiseFrameOIDN(output.color, output.albedo, output.normal, false) : output.color;
                auto previewSRGB = hdrToSRGB(preview, gamma);
                writeBMP(OUTPUT_FOLDER / "preview.bmp", previewSRGB);

                previewNextUpdate = currentTime + PROGRESS_VIEW_UPDATE_INTERVAL;
            }
        }
    };

    // Setup scene
    std::function<std::pair<Ref<World>, Ref<Camera>>()> scenes[] = {
        /* 0 */ sphereScene,
        /* 1 */ randomSphereScene,
        /* 2 */ teapotDragonScene,
        /* 3 */ reimuScene,
        /* 4 */ sponzaScene,
        /* 5 */ normalTestScene,
    };
    u32 sceneIndex = 2;

    auto [world, camera] = scenes[sceneIndex]();

    // Render
    if (!std::filesystem::exists(OUTPUT_FOLDER))
        std::filesystem::create_directory(OUTPUT_FOLDER);

    LOG(std::format("Rendering image {}x{}", renderer.m_imageSize.x, renderer.m_imageSize.y));

    auto start = std::chrono::high_resolution_clock::now();
    Renderer::Output output = renderer.renderFrame(world, camera);
    auto stop = std::chrono::high_resolution_clock::now();

    LOG(std::format("Time taken: {:.2f}s", std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() / 1000.0));

    // Save output
    if (renderer.m_outputChannels & (u32)Renderer::OutputChannel::Color) {
        writeEXR(OUTPUT_FOLDER / "color.exr", output.color);
        writeBMP(OUTPUT_FOLDER / "color.bmp", hdrToSRGB(output.color, gamma));
    }
    if (canBeDenoised) {
        Texture<vec3> denoisedColor = denoiseFrameOIDN(output.color, output.albedo, output.normal);
        writeEXR(OUTPUT_FOLDER / "denoised.exr", denoisedColor);
        writeBMP(OUTPUT_FOLDER / "denoised.bmp", hdrToSRGB(denoisedColor, gamma));
    }
    if (renderer.m_outputChannels & (u32)Renderer::OutputChannel::Depth)
        writeEXR(OUTPUT_FOLDER / "depth.exr", output.depth);
    if (renderer.m_outputChannels & (u32)Renderer::OutputChannel::Normal)
        writeEXR(OUTPUT_FOLDER / "normal.exr", output.normal);
    if (renderer.m_outputChannels & (u32)Renderer::OutputChannel::Albedo)
        writeEXR(OUTPUT_FOLDER / "albedo.exr", output.albedo);
    if (renderer.m_outputChannels & (u32)Renderer::OutputChannel::Emission)
        writeEXR(OUTPUT_FOLDER / "emission.exr", output.emission);
#ifdef BVH_TEST
    if (renderer.m_outputChannels & (u32)Renderer::OutputChannel::AABBTestCount)
        writeEXR(OUTPUT_FOLDER / "aabb-test-count.exr", output.aabbTestCount);
    if (renderer.m_outputChannels & (u32)Renderer::OutputChannel::TriangleTestCount)
        writeEXR(OUTPUT_FOLDER / "triangle-test-count.exr", output.triangleTestCount);
#endif
}

i32 main(i32 argc, char** argv) {
    render();
    return EXIT_SUCCESS;
}
