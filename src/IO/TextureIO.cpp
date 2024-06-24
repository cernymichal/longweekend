#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define TINYEXR_IMPLEMENTATION
#define TINYEXR_USE_MINIZ 0
#include <miniz/miniz.h>
#define TINYEXR_USE_THREAD 1
#include <tinyexr.h>

#include "TextureIO.h"

template <typename T>
Texture<T> readTexture(const std::string_view filePath, bool flipVertically) {
    if (filePath.ends_with(".exr"))
        return readEXR<T>(filePath, flipVertically);
    else
        return readSTB<T>(filePath, flipVertically);
}

template Texture<f32> readTexture(const std::string_view filePath, bool flipVertically);
template Texture<vec2> readTexture(const std::string_view filePath, bool flipVertically);
template Texture<vec3> readTexture(const std::string_view filePath, bool flipVertically);
template Texture<vec4> readTexture(const std::string_view filePath, bool flipVertically);

template <typename T>
Texture<T> readSTB(const std::string_view filePath, bool flipVertically) {
    u32 channelsToLoad = 3;
    if constexpr (std::is_same_v<T, f32>)
        channelsToLoad = 1;
    else if constexpr (std::is_same_v<T, vec2>)
        channelsToLoad = 2;
    else if constexpr (std::is_same_v<T, vec3>)
        channelsToLoad = 3;
    else if constexpr (std::is_same_v<T, vec4>)
        channelsToLoad = 4;
    else
        assert(false);

    if (flipVertically)
        stbi_set_flip_vertically_on_load(true);
    else
        stbi_set_flip_vertically_on_load(false);

    i32 channels;
    ivec2 size;
    T* data = nullptr;

    const auto filePathStr = std::string(filePath);
    if (stbi_is_hdr(filePathStr.c_str()))
        data = reinterpret_cast<T*>(stbi_loadf(filePathStr.c_str(), &size.x, &size.y, &channels, channelsToLoad));
    else {
        // TODO add u8 texture support

        u8* dataU8 = stbi_load(filePathStr.c_str(), &size.x, &size.y, &channels, channelsToLoad);

        if (dataU8) {
            data = new T[size.x * size.y];
            for (size_t i = 0; i < size.x * size.y * channelsToLoad; i++)
                reinterpret_cast<f32*>(data)[i] = static_cast<f32>(dataU8[i]) / 255.0f;
        }
    }

    if (!data) {
        LOG("Failed to load texture " << filePath);
        throw std::runtime_error("Failed to load texture");
    }

    return Texture<T>(size, std::move(data));
}

template <typename T>
Texture<T> readEXR(const std::string_view filePath, bool flipVertically) {
    std::string pathString = std::string(filePath);
    LOG("Loading texture " << pathString);

    u32 channelsToLoad = 3;
    if constexpr (std::is_same_v<T, f32>)
        channelsToLoad = 1;
    else if constexpr (std::is_same_v<T, vec2>)
        channelsToLoad = 2;
    else if constexpr (std::is_same_v<T, vec3>)
        channelsToLoad = 3;
    else if constexpr (std::is_same_v<T, vec4>)
        channelsToLoad = 4;
    else
        assert(false);

    EXRVersion version;
    i32 status = ParseEXRVersionFromFile(&version, pathString.c_str());
    if (status != TINYEXR_SUCCESS || version.multipart) {
        LOG("Invalid EXR file");
        throw std::runtime_error("Invalid EXR file");
    }

    EXRHeader header;
    InitEXRHeader(&header);
    const char* error = nullptr;
    status = ParseEXRHeaderFromFile(&header, &version, pathString.c_str(), &error);

    if (status == TINYEXR_SUCCESS && header.num_channels < channelsToLoad) {
        error = "Not enough channels";
        status = -1;
    }

    if (status != TINYEXR_SUCCESS) {
        LOG("Invalid EXR file");
        LOG(error);
        FreeEXRErrorMessage(error);
        throw std::runtime_error("Invalid EXR file");
    }

    // Read HALF channel as f32.
    for (u32 i = 0; i < header.num_channels; i++) {
        if (header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF)
            header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
    }

    EXRImage image;
    InitEXRImage(&image);
    status = LoadEXRImageFromFile(&image, &header, pathString.c_str(), &error);
    if (status != 0) {
        LOG("Couldn't load EXR file");
        LOG(error);
        FreeEXRHeader(&header);
        FreeEXRErrorMessage(error);
        throw std::runtime_error("Couldn't load EXR file");
    }

    ivec2 size = ivec2(image.width, image.height);
    T* data = new T[size.x * size.y];

    // Load the image data to a single array, flipping it vertically if necessary
    f32* dataf32 = reinterpret_cast<f32*>(data);
    for (u32 channel = 0; channel < channelsToLoad; channel++) {
        auto idx = uvec2(0);
        for (idx.y = 0; idx.y < size.y; idx.y++) {
            for (idx.x = 0; idx.x < size.x; idx.x++) {
                auto i = idx.y * size.x + idx.x;
                auto flippedI = (flipVertically ? size.y - 1 - idx.y : idx.y) * size.x + idx.x;
                dataf32[flippedI * channelsToLoad + channel] = reinterpret_cast<f32*>(image.images[channel])[i];
            }
        }
    }

    // Swap R and B channels because of BGRA format
    if (channelsToLoad >= 3) {
        auto idx = uvec2(0);
        for (idx.y = 0; idx.y < size.y; idx.y++) {
            for (idx.x = 0; idx.x < size.x; idx.x++) {
                auto i = (idx.y * size.x + idx.x) * channelsToLoad;
                std::swap(dataf32[i], dataf32[i + 2]);
            }
        }
    }

    FreeEXRImage(&image);
    FreeEXRHeader(&header);

    return Texture<T>(size, std::move(data));
}

void writeBMP(const std::string_view filePath, const Texture<u8vec3> texture, bool flipVertically) {
    if (flipVertically)
        stbi_flip_vertically_on_write(true);
    else
        stbi_flip_vertically_on_write(false);

    std::string pathString = std::string(filePath);
    stbi_write_bmp(pathString.c_str(), texture.size().x, texture.size().y, 3, texture.data());
}

template <typename T>
void writeEXR(const std::string_view filePath, const Texture<T> texture, bool flipVertically) {
    std::string pathString = std::string(filePath);
    LOG("Saving texture " << pathString);

    u32 channelsToSave = 1;
    if constexpr (std::is_same_v<T, f32>)
        channelsToSave = 1;
    else if constexpr (std::is_same_v<T, vec2>)
        channelsToSave = 2;
    else if constexpr (std::is_same_v<T, vec3>)
        channelsToSave = 3;
    else if constexpr (std::is_same_v<T, vec4>)
        channelsToSave = 4;
    else
        assert(false);

    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    std::vector<f32*> images;
    for (u32 i = 0; i < channelsToSave; i++)
        images.push_back(new f32[texture.size().x * texture.size().y]);

    const f32* dataf32 = reinterpret_cast<const f32*>(texture.data());
    auto idx = uvec2(0);
    for (idx.y = 0; idx.y < texture.size().y; idx.y++) {
        for (idx.x = 0; idx.x < texture.size().x; idx.x++) {
            auto i = idx.y * texture.size().x + idx.x;
            auto flippedI = (flipVertically ? texture.size().y - 1 - idx.y : idx.y) * texture.size().x + idx.x;
            for (u32 channel = 0; channel < channelsToSave; channel++)
                images[channel][flippedI] = dataf32[i * channelsToSave + channel];
        }
    }

    // Must be BGR(A) order, since most of EXR viewers expect this channel order.
    if (channelsToSave >= 3)
        std::swap(images[0], images[2]);

    image.images = reinterpret_cast<u8**>(images.data());
    image.width = texture.size().x;
    image.height = texture.size().y;
    image.num_channels = channelsToSave;

    std::vector<EXRChannelInfo> channels(channelsToSave, EXRChannelInfo());
    const char* channelNames[] = {"B", "G", "R", "A"};
    for (u32 i = 0; i < channelsToSave; i++) {
        auto nameLength = strlen(channelNames[i]);
        for (u32 j = 0; j < nameLength; j++)
            channels[i].name[j] = channelNames[i][j];
        channels[i].name[nameLength] = '\0';
    }
    if (channelsToSave < 3)
        header.channels[0].name[0] = 'R';

    std::vector<i32> pixelTypes(channelsToSave, TINYEXR_PIXELTYPE_FLOAT);
    std::vector<i32> requestedPixelTypes(channelsToSave, TINYEXR_PIXELTYPE_HALF);

    header.channels = channels.data();
    header.num_channels = image.num_channels;
    header.pixel_types = pixelTypes.data();
    header.requested_pixel_types = requestedPixelTypes.data();
    header.compression_type = TINYEXR_COMPRESSIONTYPE_ZIP;  // TINYEXR_COMPRESSIONTYPE_PIZ;

    const char* error;
    i32 status = SaveEXRImageToFile(&image, &header, pathString.c_str(), &error);

    for (i32 i = 0; i < images.size(); i++)
        delete[] images[i];

    if (status != TINYEXR_SUCCESS) {
        LOG("Saving texture failed");
        LOG(error);
        FreeEXRErrorMessage(error);
        throw std::runtime_error("Saving texture failed");
    }
}

template void writeEXR(const std::string_view filePath, const Texture<f32> texture, bool flipVertically);
template void writeEXR(const std::string_view filePath, const Texture<vec2> texture, bool flipVertically);
template void writeEXR(const std::string_view filePath, const Texture<vec3> texture, bool flipVertically);
template void writeEXR(const std::string_view filePath, const Texture<vec4> texture, bool flipVertically);
