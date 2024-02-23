#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

Texture::Texture(const std::filesystem::path& filePath) {
    LOG("Loading image " << filePath);

    int channels;
    ivec2 sizeInt;

    const auto filePathStr = filePath.string();
    void* imageData;
    imageData = stbi_loadf(filePathStr.c_str(), &sizeInt.x, &sizeInt.y, &channels, 3);

    if (!imageData) {
        LOG("Failed to load texture " << filePath);
        throw std::runtime_error("Failed to load texture");
    }

    size = sizeInt;
    data = reinterpret_cast<vec3*>(imageData);
}

Texture::~Texture() {
    stbi_image_free(data);
}
