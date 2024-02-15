// #include <windows.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "Camera.h"
#include "HittableGroup.h"
#include "Sphere.h"

constexpr auto OUTPUT_FILENAME = "output.bmp";

int main(int argc, char** argv) {
    // camera
    auto imageSize = uvec2(640, 480);  // / 2U;
    Camera camera;
    camera.m_imageSize = imageSize;
    camera.m_samples = 128;
    camera.m_maxBounces = 32;

    // world
    HittableGroup world;
    for (int i = 0; i < 10; i++) {
        auto position = vec3(randomFloat(-2, 2), randomFloat(-2, 2), randomFloat(-5, 0));
        auto radius = randomFloat(0.2f, 1);
        world.add(makeRef<Sphere>(position, radius));
    }

    // render
    std::vector<glm::u8vec3> framebuffer(imageSize.x * imageSize.y);
    camera.render(world, framebuffer);

    // output image
    stbi_write_bmp(OUTPUT_FILENAME, imageSize.x, imageSize.y, 3, framebuffer.data());
    LOG("image written");
    // ShellExecute(0, 0, L"output.bmp", 0, 0, SW_SHOW);

    return EXIT_SUCCESS;
}
