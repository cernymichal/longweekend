#pragma once

#include "Texture.h"

template <typename T>
Texture<T> readTexture(const std::string_view filePath, bool flipVertically = false);

template <typename T>
Texture<T> readSTB(const std::string_view filePath, bool flipVertically = false);

template <typename T>
Texture<T> readEXR(const std::string_view filePath, bool flipVertically = false);

void writeBMP(const std::string_view filePath, const Texture<u8vec3> texture, bool flipVertically = false);

template <typename T>
void writeEXR(const std::string_view filePath, const Texture<T> texture, bool flipVertically = false);
