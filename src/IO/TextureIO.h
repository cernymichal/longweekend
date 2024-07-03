#pragma once

#include "Texture.h"

template <typename T>
Texture<T> loadTexture(const std::filesystem::path& filePath, bool flipVertically = false);

template <typename T>
Texture<T> loadTextureSTB(const std::filesystem::path& filePath, bool flipVertically = false);

template <typename T>
Texture<T> loadEXR(const std::filesystem::path& filePath, bool flipVertically = false);

void writeBMP(const std::filesystem::path& filePath, const Texture<u8vec3> texture, bool flipVertically = false);

template <typename T>
void writeEXR(const std::filesystem::path& filePath, const Texture<T> texture, bool flipVertically = false);
