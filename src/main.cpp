#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "Camera.h"
#include "HittableGroup.h"
#include "Sphere.h"

constexpr auto OUTPUT_FILENAME = "output.bmp";

int main(int argc, char** argv) {
    // camera
    Camera camera;
    camera.m_position = vec3(-2, 2, 1);
    camera.m_lookAt = vec3(-.15, 0, -1);
    camera.m_fov = 35.0f;
    camera.m_imageSize = uvec2(640, 480);
    camera.m_samples = 64;
    camera.m_maxBounces = 16;

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
    for (int i = 0; i < 10; i++) {
        auto position = vec3(randomFloat(-2, 2), randomFloat(-2, 2), randomFloat(-5, 0));
        auto radius = randomFloat(0.2f, 1);
        world.add(makeRef<Sphere>(position, radius, centerMaterial));
    }
    */

    // render
    std::vector<glm::u8vec3> framebuffer(camera.m_imageSize.x * camera.m_imageSize.y);
    auto start = std::chrono::high_resolution_clock::now();
    camera.render(world, framebuffer);
    auto stop = std::chrono::high_resolution_clock::now();

    LOG(std::format("Time taken: {:.2}s", std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() / 1000.0));

    // output image
    stbi_write_bmp(OUTPUT_FILENAME, camera.m_imageSize.x, camera.m_imageSize.y, 3, framebuffer.data());
    LOG("image written");

    return EXIT_SUCCESS;
}
