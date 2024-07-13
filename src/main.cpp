#include "Camera.h"
#include "Hittables/HittableGroup.h"
#include "Hittables/Sphere.h"
#include "IO/MeshIO.h"
#include "IO/TextureIO.h"
#include "Postprocessing.h"
#include "Texture.h"

constexpr bool ENABLE_PREVIEW = true;
constexpr bool DENOISE_PREVIEW = true;
constexpr auto PROGRESS_VIEW_UPDATE_INTERVAL = std::chrono::seconds(2);
const std::filesystem::path OUTPUT_FOLDER = "output";

void sphereScene(HittableGroup& world, Camera& camera) {
    // camera
    camera.m_position = vec3(-2, .2, 1);
    camera.m_lookAt = vec3(-.15, .4, -1);
    camera.m_fov = 60.0f;
    camera.m_defocusAngle = 3.0f;
    camera.m_focusDistance = glm::length(camera.m_position - camera.m_lookAt);

    camera.m_environmentMaterial->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>("resources/evening_field_1k.exr"));

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

    world.add(makeRef<Sphere>(vec3(0.0, -100.5, -1.0), 100.0f, groundMaterial));
    world.add(makeRef<Sphere>(vec3(0.0, 0.0, -1), 0.5f, centerMaterial));
    world.add(makeRef<Sphere>(vec3(-1.0, 0.0, -1), 0.5f, leftMaterial));
    world.add(makeRef<Sphere>(vec3(-1.0, 0.0, -1), -0.4f, leftMaterial));
    world.add(makeRef<Sphere>(vec3(1.0, 0.0, -1), 0.5f, rightMaterial));
}

void randomSphereScene(HittableGroup& world, Camera& camera) {
    // camera
    camera.m_position = vec3(13, 2, 3);
    camera.m_lookAt = vec3(0, 0, 0);
    camera.m_fov = 20.0f;
    camera.m_defocusAngle = 0.6f;
    camera.m_focusDistance = 10.0f;

    camera.m_environmentMaterial->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto groundMaterial = makeRef<Material>();
    *groundMaterial = {
        .albedo = vec3(0.5, 0.5, 0.5),
    };
    world.add(makeRef<Sphere>(vec3(0, -1000, 0), 1000, groundMaterial));

    for (i32 a = -11; a < 11; a++) {
        for (i32 b = -11; b < 11; b++) {
            auto chooseMat = random<f64>();
            vec3 center(a + 0.9 * random<f64>(), 0.2, b + 0.9 * random<f64>());

            if ((center - vec3(4, 0.2, 0)).length() > 0.9) {
                auto sphereMaterial = makeRef<Material>();

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
                        .emission = randomVec<3>() * randomVec<3>(),
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

                world.add(makeRef<Sphere>(center, 0.2, sphereMaterial));
            }
        }
    }

    auto material1 = makeRef<Material>();
    *material1 = {
        .ir = 1.5f,
        .scatterFunction = dielectricScatter,
    };
    world.add(makeRef<Sphere>(vec3(0, 1, 0), 1.0, material1));

    auto material2 = makeRef<Material>();
    *material2 = {
        .albedo = vec3(0.4, 0.2, 0.1),
    };
    world.add(makeRef<Sphere>(vec3(-4, 1, 0), 1.0, material2));

    auto material3 = makeRef<Material>();
    *material3 = {
        .albedo = vec3(0.7, 0.6, 0.5),
        .fuzziness = 0.0f,
        .scatterFunction = metallicScatter,
    };
    world.add(makeRef<Sphere>(vec3(4, 1, 0), 1.0, material3));
}

void teapotDragonScene(HittableGroup& world, Camera& camera) {
    // camera
    camera.m_position = vec3(0, 0.1, 0.7);
    camera.m_lookAt = vec3(0, 0, 0);
    camera.m_fov = 48.0f;

    camera.m_environmentMaterial->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto teapotMesh = makeRef<Mesh>(loadOBJ("resources/teapot.obj"));
    *teapotMesh->m_submeshes[0].material = {
        .ir = 1.5f,
        .scatterFunction = dielectricScatter,
    };
    auto teapot = makeRef<Model>(teapotMesh);

    teapot->m_transform.scale(vec3(1.5));
    teapot->m_transform.move(vec3(-0.12, -0.1, 0.3));
    teapot->m_transform.rotate(glm::radians(vec3(0, -20, 0)));

    world.add(teapot);

    auto dragonMesh = makeRef<Mesh>(loadOBJ("resources/dragon.obj"));
    *dragonMesh->m_submeshes[0].material = {
        .albedo = vec3(0.5, 0.6, 0.8),
    };
    auto dragon = makeRef<Model>(dragonMesh);

    dragon->m_transform.scale(vec3(0.6));
    dragon->m_transform.move(vec3(0.1, 0.02, 0.0));
    dragon->m_transform.rotate(glm::radians(vec3(0, 110, 0)));

    world.add(dragon);

    auto sphere_material = makeRef<Material>();
    *sphere_material = {
        .albedo = vec3(0),
        .emission = vec3(1),
        .emissionIntensity = 10.0f,
    };
    auto sphere = makeRef<Sphere>(vec3(-0.1, 0.15, 0.1), 0.02, sphere_material);

    world.add(sphere);

    camera.m_defocusAngle = 0.6f;
    camera.m_focusDistance = glm::distance(camera.m_position, sphere->m_center) - 0.1f;
}

void tetrahedronScene(HittableGroup& world, Camera& camera) {
    // camera
    camera.m_position = vec3(-0.1, -0.3, 2);
    camera.m_lookAt = vec3(0.3, 0.3, 0.3);
    camera.m_fov = 40.0f;
    // camera.m_defocusAngle = 0.6f;
    // camera.m_focusDistance = 10.0f;

    camera.m_environmentMaterial->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto tetrahedronMesh = makeRef<Mesh>(loadOBJ("resources/tetrahedron.obj"));
    auto tetrahedron = makeRef<Model>(tetrahedronMesh);

    world.add(tetrahedron);
}

void reimuScene(HittableGroup& world, Camera& camera) {
    // camera
    camera.m_position = vec3(0, 0, 2);
    camera.m_lookAt = vec3(0, 0, 0);
    camera.m_fov = 48.0f;

    camera.m_environmentMaterial->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto reimuMesh = makeRef<Mesh>(loadOBJ("resources/reimu/reimu.obj"));
    auto reimu = makeRef<Model>(reimuMesh);

    reimu->m_transform.scale(vec3(1.0 / 20.0));
    reimu->m_transform.move(vec3(0.5, 0.15, 0.5));
    reimu->m_transform.rotate(glm::radians(vec3(0, -90, 0)));

    world.add(reimu);
}

void sponzaScene(HittableGroup& world, Camera& camera) {
    // camera
    camera.m_position = vec3(8, 1.5, 0);
    camera.m_lookAt = vec3(6, 1.7, 0);
    camera.m_fov = 50.0f;

    camera.m_environmentMaterial->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto sponzaMesh = makeRef<Mesh>(loadOBJ("resources/sponza/sponza.obj"));
    auto sponza = makeRef<Model>(sponzaMesh);

    sponza->m_transform.scale(vec3(1.0 / 100.0));

    world.add(sponza);
}

void normalTestScene(HittableGroup& world, Camera& camera) {
    // camera
    camera.m_position = vec3(0, 0, 2);
    camera.m_lookAt = vec3(0, 0, 0);
    camera.m_fov = 48.0f;

    camera.m_environmentMaterial->emissionTexture = makeRef<Texture<vec3>>(loadTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto cubeMesh = makeRef<Mesh>(loadOBJ("resources/normal_test/normal_test.obj"));
    auto cube = makeRef<Model>(cubeMesh);

    cube->m_transform.scale(vec3(1.0 / 2.0));
    cube->m_transform.rotate(glm::radians(vec3(30, -30, 0)));

    world.add(cube);
}

void render() {
    HittableGroup world;
    Camera camera;

    camera.m_imageSize = uvec2(640, 480);
    camera.m_samples = 64;
    camera.m_auxillarySamples = 16;
    camera.m_maxBounces = 8;
    f32 gamma = 2.2f;

    camera.m_outputChannels = (u32)OutputChannel::Color | (u32)OutputChannel::Albedo | (u32)OutputChannel::Normal;

    // randomSphereScene(world, camera);
    // sphereScene(world, camera);
    teapotDragonScene(world, camera);
    // tetrahedronScene(world, camera);
    // reimuScene(world, camera);
    // sponzaScene(world, camera);
    // normalTestScene(world, camera);

    u32 denoiseChannels = (u32)OutputChannel::Color | (u32)OutputChannel::Albedo | (u32)OutputChannel::Normal;
    bool canBeDenoised = (camera.m_outputChannels & denoiseChannels) == denoiseChannels;
    if (DENOISE_PREVIEW && !canBeDenoised)
        LOG("Denoising preview is disabled because the output channels do not include color, albedo, or normal");

    // render
    if (!std::filesystem::exists(OUTPUT_FOLDER))
        std::filesystem::create_directory(OUTPUT_FOLDER);

    auto auxillarySampleCallback = [&](const RenderOuput& output, u32 sample) {
        LOG(std::format("{}/{} auxillary samples done ({:.1f}%)", sample, camera.m_auxillarySamples, sample * 100.0f / camera.m_auxillarySamples));

        if (sample == camera.m_auxillarySamples) {
            if (camera.m_outputChannels & (u32)OutputChannel::Depth)
                writeEXR(OUTPUT_FOLDER / "depth.exr", output.depth);
            if (camera.m_outputChannels & (u32)OutputChannel::Normal)
                writeEXR(OUTPUT_FOLDER / "normal.exr", output.normal);
            if (camera.m_outputChannels & (u32)OutputChannel::Albedo)
                writeEXR(OUTPUT_FOLDER / "albedo.exr", output.albedo);
            if (camera.m_outputChannels & (u32)OutputChannel::Emission)
                writeEXR(OUTPUT_FOLDER / "emission.exr", output.emission);
#ifdef BVH_TEST
            if (camera.m_outputChannels & (u32)OutputChannel::AABBTestCount)
                writeEXR(OUTPUT_FOLDER / "aabb-test-count.exr", output.aabbTestCount);
            if (camera.m_outputChannels & (u32)OutputChannel::FaceTestCount)
                writeEXR(OUTPUT_FOLDER / "face-test-count.exr", output.faceTestCount);
#endif
        }
    };

    auto previewNextUpdate = std::chrono::high_resolution_clock::now();
    auto colorSampleCallback = [&](const RenderOuput& output, u32 sample) {
        LOG(std::format("{}/{} samples done ({:.1f}%)", sample, camera.m_samples, sample * 100.0f / camera.m_samples));

        if (ENABLE_PREVIEW && camera.m_outputChannels & (u32)OutputChannel::Color) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            if (previewNextUpdate <= currentTime) {
                auto preview = DENOISE_PREVIEW && canBeDenoised ? denoiseFrameOIDN(output.color, output.albedo, output.normal, false) : output.color;
                auto previewSRGB = hdrToSRGB(preview, gamma);
                writeBMP(OUTPUT_FOLDER / "preview.bmp", previewSRGB);

                previewNextUpdate = currentTime + PROGRESS_VIEW_UPDATE_INTERVAL;
            }
        }
    };

    LOG(std::format("Rendering image {}x{}", camera.m_imageSize.x, camera.m_imageSize.y));

    auto start = std::chrono::high_resolution_clock::now();
    RenderOuput output = camera.render(world, colorSampleCallback, auxillarySampleCallback);
    auto stop = std::chrono::high_resolution_clock::now();

    LOG(std::format("Time taken: {:.2f}s", std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() / 1000.0));

    if (camera.m_outputChannels & (u32)OutputChannel::Color) {
        writeEXR(OUTPUT_FOLDER / "color.exr", output.color);
        writeBMP(OUTPUT_FOLDER / "color.bmp", hdrToSRGB(output.color, gamma));
    }
    if (canBeDenoised) {
        Texture<vec3> denoisedColor = denoiseFrameOIDN(output.color, output.albedo, output.normal);
        writeEXR(OUTPUT_FOLDER / "denoised.exr", denoisedColor);
        writeBMP(OUTPUT_FOLDER / "denoised.bmp", hdrToSRGB(denoisedColor, gamma));
    }
}

i32 main(i32 argc, char** argv) {
    render();
    return EXIT_SUCCESS;
}
