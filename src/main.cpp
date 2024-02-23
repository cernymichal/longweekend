#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "Camera.h"
#include "HittableGroup.h"
#include "Sphere.h"
#include "Texture.h"

constexpr auto OUTPUT_FILENAME = "output.bmp";

int main(int argc, char** argv) {
    // camera
    Camera camera;
    camera.m_position = vec3(-2, .2, 1);
    camera.m_lookAt = vec3(-.15, .4, -1);
    camera.m_fov = 60.0f;
    camera.m_defocusAngle = 3.0f;
    camera.m_focusDistance = glm::length(camera.m_position - camera.m_lookAt);
    camera.m_imageSize = uvec2(640, 480);
    camera.m_samples = 128;
    camera.m_maxBounces = 16;

    camera.m_environment = makeRef<Texture>("resources/evening_field_2k.hdr");

    // world
    HittableGroup world;

    auto groundMaterial = makeRef<LambertianMaterial>(vec3(0.8, 0.8, 0.0));
    auto centerMaterial = makeRef<LambertianMaterial>(vec3(0.1, 0.2, 0.5));
    auto leftMaterial = makeRef<DielectricMaterial>(1.5f);
    auto rightMaterial = makeRef<MetalMaterial>(vec3(0.8, 0.6, 0.2), 0.0f);

    world.add(makeRef<Sphere>(vec3(0.0, -100.5, -1.0), 100.0f, groundMaterial));
    world.add(makeRef<Sphere>(vec3(0.0, 0.0, -1), 0.5f, centerMaterial));
    world.add(makeRef<Sphere>(vec3(-1.0, 0.0, -1), 0.5f, leftMaterial));
    world.add(makeRef<Sphere>(vec3(-1.0, 0.0, -1), -0.4f, leftMaterial));
    world.add(makeRef<Sphere>(vec3(1.0, 0.0, -1), 0.5f, rightMaterial));

    // random spheres
    /*
    HittableGroup world;

    auto ground_material = makeRef<LambertianMaterial>(vec3(0.5, 0.5, 0.5));
    world.add(makeRef<Sphere>(vec3(0, -1000, 0), 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random<double>();
            vec3 center(a + 0.9 * random<double>(), 0.2, b + 0.9 * random<double>());

            if ((center - vec3(4, 0.2, 0)).length() > 0.9) {
                Ref<Material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = randomVec<3>() * randomVec<3>();
                    sphere_material = makeRef<LambertianMaterial>(albedo);
                    world.add(makeRef<Sphere>(center, 0.2, sphere_material));
                }
                else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = randomVec<3>(vec3(0.5), vec3(1));
                    auto fuzz = random<double>(0, 0.5);
                    sphere_material = makeRef<MetalMaterial>(albedo, fuzz);
                    world.add(makeRef<Sphere>(center, 0.2, sphere_material));
                }
                else {
                    // glass
                    sphere_material = makeRef<DielectricMaterial>(1.5);
                    world.add(makeRef<Sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = makeRef<DielectricMaterial>(1.5);
    world.add(makeRef<Sphere>(vec3(0, 1, 0), 1.0, material1));

    auto material2 = makeRef<LambertianMaterial>(vec3(0.4, 0.2, 0.1));
    world.add(makeRef<Sphere>(vec3(-4, 1, 0), 1.0, material2));

    auto material3 = makeRef<MetalMaterial>(vec3(0.7, 0.6, 0.5), 0.0);
    world.add(makeRef<Sphere>(vec3(4, 1, 0), 1.0, material3));

    Camera camera;
    camera.m_position = vec3(13, 2, 3);
    camera.m_lookAt = vec3(0, 0, 0);
    camera.m_fov = 20.0f;
    camera.m_defocusAngle = 0.6f;
    camera.m_focusDistance = 10.0f;
    camera.m_imageSize = uvec2(1200, 675);
    camera.m_samples = 128;
    camera.m_maxBounces = 32;

    camera.m_environment = makeRef<Texture>("resources/evening_field_2k.hdr");
    */

    // render
    std::vector<glm::u8vec3> framebuffer(camera.m_imageSize.x * camera.m_imageSize.y);
    auto start = std::chrono::high_resolution_clock::now();
    camera.render(world, framebuffer);
    auto stop = std::chrono::high_resolution_clock::now();

    LOG(std::format("Time taken: {:.2f}s", std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() / 1000.0));

    // output image
    stbi_write_bmp(OUTPUT_FILENAME, camera.m_imageSize.x, camera.m_imageSize.y, 3, framebuffer.data());
    LOG("image written");

    return EXIT_SUCCESS;
}
