#pragma once

struct Texture {
    uvec2 size = uvec2(0);
    vec3* data = nullptr;

    Texture(const std::filesystem::path& filePath);

    ~Texture();
};
