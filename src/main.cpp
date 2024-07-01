#include "Camera.h"
#include "Hittables/HittableGroup.h"
#include "Hittables/Sphere.h"
#include "IO/MeshIO.h"
#include "IO/TextureIO.h"
#include "Postprocessing.h"
#include "Texture.h"

constexpr bool ENABLE_PROGRESS_VIEW = true;
constexpr auto PROGRESS_VIEW_FILENAME = "progress_view.bmp";
constexpr auto OUTPUT_FILENAME = "output.exr";

void sphereScene(HittableGroup& world, Camera& camera) {
    // camera
    camera.m_position = vec3(-2, .2, 1);
    camera.m_lookAt = vec3(-.15, .4, -1);
    camera.m_fov = 60.0f;
    camera.m_defocusAngle = 3.0f;
    camera.m_focusDistance = glm::length(camera.m_position - camera.m_lookAt);

    camera.m_environment = makeRef<Texture<vec3>>(readTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto groundMaterial = makeRef<LambertianMaterial>(vec3(0.8, 0.8, 0.0));
    auto centerMaterial = makeRef<LambertianMaterial>(vec3(0.1, 0.2, 0.5));
    auto leftMaterial = makeRef<DielectricMaterial>(1.5f);
    auto rightMaterial = makeRef<MetalMaterial>(vec3(0.8, 0.6, 0.2), 0.0f);

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

    camera.m_environment = makeRef<Texture<vec3>>(readTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto ground_material = makeRef<LambertianMaterial>(vec3(0.5, 0.5, 0.5));
    world.add(makeRef<Sphere>(vec3(0, -1000, 0), 1000, ground_material));

    for (i32 a = -11; a < 11; a++) {
        for (i32 b = -11; b < 11; b++) {
            auto chooseMat = random<f64>();
            vec3 center(a + 0.9 * random<f64>(), 0.2, b + 0.9 * random<f64>());

            if ((center - vec3(4, 0.2, 0)).length() > 0.9) {
                Ref<Material> sphere_material;

                if (chooseMat < 0.7) {
                    // diffuse
                    auto albedo = randomVec<3>() * randomVec<3>();
                    sphere_material = makeRef<LambertianMaterial>(albedo);
                }
                else if (chooseMat < 0.8) {
                    // metal
                    auto albedo = randomVec<3>(vec3(0.5), vec3(1));
                    auto fuzz = random<f64>(0, 0.5);
                    sphere_material = makeRef<MetalMaterial>(albedo, fuzz);
                }
                else if (chooseMat < 0.9) {
                    // emissive
                    auto albedo = vec3(0);
                    auto emission = randomVec<3>() * randomVec<3>();
                    f32 intensity = random<f32>(10, 50);
                    sphere_material = makeRef<LambertianMaterial>(albedo, emission, intensity);
                }
                else {
                    // glass
                    sphere_material = makeRef<DielectricMaterial>(1.5);
                }

                world.add(makeRef<Sphere>(center, 0.2, sphere_material));
            }
        }
    }

    auto material1 = makeRef<DielectricMaterial>(1.5);
    world.add(makeRef<Sphere>(vec3(0, 1, 0), 1.0, material1));

    auto material2 = makeRef<LambertianMaterial>(vec3(0.4, 0.2, 0.1));
    world.add(makeRef<Sphere>(vec3(-4, 1, 0), 1.0, material2));

    auto material3 = makeRef<MetalMaterial>(vec3(0.7, 0.6, 0.5), 0.0);
    world.add(makeRef<Sphere>(vec3(4, 1, 0), 1.0, material3));
}

void teapotScene(HittableGroup& world, Camera& camera) {
    // camera
    camera.m_position = vec3(0.07, 0.08, 0.13);
    camera.m_lookAt = vec3(0.015, 0.035, 0.0);
    camera.m_fov = 48.0f;
    // camera.m_defocusAngle = 0.1f;
    // camera.m_focusDistance = 0.15f;

    camera.m_environment = makeRef<Texture<vec3>>(readTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto teapot = makeRef<Mesh>(loadOBJ("resources/teapot.obj"));

    world.add(teapot);
}

void tetrahedronScene(HittableGroup& world, Camera& camera) {
    // camera
    camera.m_position = vec3(-0.1, -0.3, 2);
    camera.m_lookAt = vec3(0.3, 0.3, 0.3);
    camera.m_fov = 40.0f;
    // camera.m_defocusAngle = 0.6f;
    // camera.m_focusDistance = 10.0f;

    camera.m_environment = makeRef<Texture<vec3>>(readTexture<vec3>("resources/evening_field_1k.exr"));

    // world
    auto tetrahedron = makeRef<Mesh>(loadOBJ("resources/tetrahedron.obj"));

    world.add(tetrahedron);
}

void render() {
    HittableGroup world;
    Camera camera;

    camera.m_imageSize = uvec2(640, 480);  // / 2U;
    camera.m_samples = 32;
    camera.m_maxBounces = 8;
    f32 gamma = 2.2f;

    randomSphereScene(world, camera);
    // sphereScene(world, camera);
    // teapotScene(world, camera);
    // tetrahedronScene(world, camera);

    // render
    auto sampleFinishCallback = [camera, gamma](const Texture<vec3>& accumulator, u32 sample) {
        LOG(std::format("{}/{} samples done ({:.1f}%)", sample, camera.m_samples, (sample + 1) * 100.0f / camera.m_samples));

        if (ENABLE_PROGRESS_VIEW) {
            auto frameSRGB = hdrToSRGB(accumulator, gamma, sample);
            writeBMP(PROGRESS_VIEW_FILENAME, frameSRGB);
        }
    };

    LOG(std::format("Rendering image {}x{}", camera.m_imageSize.x, camera.m_imageSize.y));

    auto start = std::chrono::high_resolution_clock::now();
    Texture<vec3> frame = camera.render(world, sampleFinishCallback);
    auto stop = std::chrono::high_resolution_clock::now();

    LOG(std::format("Time taken: {:.2f}s", std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() / 1000.0));

    writeEXR(OUTPUT_FILENAME, frame);
}

i32 main(i32 argc, char** argv) {
    render();
    return EXIT_SUCCESS;
}
