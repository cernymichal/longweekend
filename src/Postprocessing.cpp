#include "Postprocessing.h"

#include <OpenImageDenoise/oidn.hpp>

struct OIDNFilterCache {
    uvec2 size = uvec2(0);
    oidn::Quality quality = oidn::Quality::Fast;
    oidn::FilterRef colorFilter;
    oidn::FilterRef albedoFilter;
    oidn::FilterRef normalFilter;
};

Texture<vec3> denoiseFrameOIDN(const Texture<vec3>& color, const Texture<vec3>& albedo, const Texture<vec3>& normal, bool highQuality) {
    // Create an Open Image Denoise device
    static oidn::DeviceRef device = oidn::newDevice();  // CPU or GPU if available
    static bool deviceCommitted = false;
    if (!deviceCommitted) {
        device.commit();
        deviceCommitted = true;
    }

    const char* filterType = "RT";
    oidn::Quality quality = highQuality ? oidn::Quality::High : oidn::Quality::Fast;
    bool prefilterAuxiliary = quality == oidn::Quality::High;

    // Create buffers for input/output images accessible by both host (CPU) and device (CPU/GPU)
    size_t bufferSize = color.size().x * color.size().y * sizeof(vec3);
    oidn::BufferRef colorBuf = device.newBuffer(bufferSize);
    oidn::BufferRef albedoBuf = device.newBuffer(bufferSize);
    oidn::BufferRef normalBuf = device.newBuffer(bufferSize);

    // Fill the input image buffers
    colorBuf.write(0, bufferSize, (void*)color.data());
    albedoBuf.write(0, bufferSize, (void*)albedo.data());
    normalBuf.write(0, bufferSize, (void*)normal.data());

    static OIDNFilterCache cache;
    if (color.size() != cache.size || quality != cache.quality) {
        // Create new filters if the image size or quality has changed
        cache.size = color.size();
        cache.quality = quality;

        // Create a filter for denoising a beauty (color) image using optional auxiliary images too
        cache.colorFilter = device.newFilter(filterType);
        cache.colorFilter.set("hdr", true);
        cache.colorFilter.set("quality", cache.quality);
        cache.colorFilter.set("cleanAux", prefilterAuxiliary);

        if (prefilterAuxiliary) {
            cache.albedoFilter = device.newFilter(filterType);  // same filter type as for beauty
            cache.normalFilter = device.newFilter(filterType);  // same filter type as for beauty
        }
    }

    // Set the input and output images
    cache.colorFilter.setImage("color", colorBuf, oidn::Format::Float3, color.size().x, color.size().y);
    cache.colorFilter.setImage("albedo", albedoBuf, oidn::Format::Float3, albedo.size().x, albedo.size().y);
    cache.colorFilter.setImage("normal", normalBuf, oidn::Format::Float3, normal.size().x, normal.size().y);
    cache.colorFilter.setImage("output", colorBuf, oidn::Format::Float3, color.size().x, color.size().y);
    cache.colorFilter.commit();

    if (prefilterAuxiliary) {
        // Use a separate filter for denoising an auxiliary albedo image (in-place)
        cache.albedoFilter.setImage("albedo", albedoBuf, oidn::Format::Float3, albedo.size().x, albedo.size().y);
        cache.albedoFilter.setImage("output", albedoBuf, oidn::Format::Float3, albedo.size().x, albedo.size().y);
        cache.albedoFilter.commit();

        // Use a separate filter for denoising an auxiliary normal image (in-place)
        cache.normalFilter.setImage("normal", normalBuf, oidn::Format::Float3, normal.size().x, normal.size().y);
        cache.normalFilter.setImage("output", normalBuf, oidn::Format::Float3, normal.size().x, normal.size().y);
        cache.normalFilter.commit();

        // Prefilter the auxiliary images
        cache.albedoFilter.execute();
        cache.normalFilter.execute();
    }

    // Filter the color image
    cache.colorFilter.execute();

    // Unset input buffers to release the memory
    cache.colorFilter.unsetImage("color");
    cache.colorFilter.unsetImage("albedo");
    cache.colorFilter.unsetImage("normal");
    cache.colorFilter.unsetImage("output");
    if (prefilterAuxiliary) {
        cache.albedoFilter.unsetImage("albedo");
        cache.albedoFilter.unsetImage("output");
        cache.normalFilter.unsetImage("normal");
        cache.normalFilter.unsetImage("output");
    }

    // Check for errors
    const char* errorMessage;
    if (device.getError(errorMessage) != oidn::Error::None)
        LOG(std::format("OIDN error: {}", errorMessage));

    // Read the output image back
    Texture<vec3> denoisedColor(color.size());
    colorBuf.read(0, bufferSize, (void*)denoisedColor.data());

    return denoisedColor;
}
