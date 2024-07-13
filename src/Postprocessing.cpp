#include "Postprocessing.h"

#include <OpenImageDenoise/oidn.hpp>

Texture<vec3> denoiseTextureOIDN(const Texture<vec3>& color, const Texture<vec3>& albedo, const Texture<vec3>& normal, bool topQuality) {
    // Create an Open Image Denoise device
    oidn::DeviceRef device = oidn::newDevice();  // CPU or GPU if available
    // oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CPU);
    device.commit();

    // Create buffers for input/output images accessible by both host (CPU) and device (CPU/GPU)
    size_t bufferSize = color.size().x * color.size().y * sizeof(vec3);
    oidn::BufferRef colorBuf;
    oidn::BufferRef albedoBuf;
    oidn::BufferRef normalBuf;

    // Fill the input image buffers
    /*
    // Don't filter in place anyway
    if (device.get<int>("type") == OIDN_DEVICE_TYPE_CPU) {
        colorBuf = device.newBuffer((void*)color.data(), bufferSize);
        albedoBuf = device.newBuffer((void*)albedo.data(), bufferSize);
        normalBuf = device.newBuffer((void*)normal.data(), bufferSize);
    }
    else {
    */
    colorBuf = device.newBuffer(bufferSize);
    albedoBuf = device.newBuffer(bufferSize);
    normalBuf = device.newBuffer(bufferSize);

    colorBuf.write(0, bufferSize, (void*)color.data());
    albedoBuf.write(0, bufferSize, (void*)albedo.data());
    normalBuf.write(0, bufferSize, (void*)normal.data());
    //}

    const char* filterType = "RT";

    // Create a filter for denoising a beauty (color) image using optional auxiliary images too
    // This can be an expensive operation, so try no to create a new filter for every image!
    oidn::FilterRef filter = device.newFilter(filterType);  // TODO cache
    filter.setImage("color", colorBuf, oidn::Format::Float3, color.size().x, color.size().y);
    filter.setImage("albedo", albedoBuf, oidn::Format::Float3, albedo.size().x, albedo.size().y);
    filter.setImage("normal", normalBuf, oidn::Format::Float3, normal.size().x, normal.size().y);
    filter.setImage("output", colorBuf, oidn::Format::Float3, color.size().x, color.size().y);
    filter.set("hdr", true);
    filter.set("quality", topQuality ? OIDN_QUALITY_HIGH : OIDN_QUALITY_FAST);
    filter.set("cleanAux", topQuality);
    filter.commit();

    if (topQuality) {
        // Create a separate filter for denoising an auxiliary albedo image (in-place)
        oidn::FilterRef albedoFilter = device.newFilter(filterType);  // same filter type as for beauty
        albedoFilter.setImage("albedo", albedoBuf, oidn::Format::Float3, albedo.size().x, albedo.size().y);
        albedoFilter.setImage("output", albedoBuf, oidn::Format::Float3, albedo.size().x, albedo.size().y);
        albedoFilter.commit();

        // Create a separate filter for denoising an auxiliary normal image (in-place)
        oidn::FilterRef normalFilter = device.newFilter(filterType);  // same filter type as for beauty
        normalFilter.setImage("normal", normalBuf, oidn::Format::Float3, normal.size().x, normal.size().y);
        normalFilter.setImage("output", normalBuf, oidn::Format::Float3, normal.size().x, normal.size().y);
        normalFilter.commit();

        // Prefilter the auxiliary images
        albedoFilter.execute();
        normalFilter.execute();
    }

    // Filter the beauty image
    filter.execute();

    // Check for errors
    const char* errorMessage;
    if (device.getError(errorMessage) != oidn::Error::None)
        LOG(std::format("OIDN error: {}", errorMessage));

    // Read the output image back
    Texture<vec3> output(color.size());
    colorBuf.read(0, bufferSize, (void*)output.data());

    return output;
}
