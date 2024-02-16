#pragma once

#include "IHittable.h"
#include "Material.h"

class Camera {
public:
    glm::uvec2 m_imageSize = glm::uvec2(256, 256);
    float m_focalLength = 1.0f;
    vec3 m_cameraCenter = vec3(0);

    unsigned m_samples = 4;
    unsigned m_maxBounces = 10;
    float m_gamma = 2.2f;

    void render(const IHittable& world, std::vector<glm::u8vec3>& target) {
        initialize();
        LOG(std::format("rendering image {0}x{1}", m_imageSize.x, m_imageSize.y));

        for (size_t y = 0; y < m_imageSize.y; y++) {
            LOG(std::format("{:.1f}% done", y * 100.0 / m_imageSize.y));

            for (size_t x = 0; x < m_imageSize.x; x++) {
                auto pixelCenter = m_pixelGridOrigin + vec3(vec2(x, y) * m_pixelDelta, 0);
                auto color = vec3(0);

                for (unsigned i = 0; i < m_samples; i++) {
                    auto jitter = vec2(randomFloat(), randomFloat()) - 0.5f;
                    auto pixelSamplePoint = pixelCenter + vec3(jitter * m_pixelDelta, 0);
                    Ray ray(m_cameraCenter, pixelSamplePoint - m_cameraCenter);

                    color += rayColor(ray, m_maxBounces, world);
                }

                target[y * m_imageSize.x + x] = rgbfromAccumulator(color);
            }
        }
        LOG("100% done");
    }

private:
    float m_aspectRatio;
    vec2 m_viewportSize;
    vec2 m_pixelDelta;
    vec3 m_pixelGridOrigin;

    void initialize() {
        m_aspectRatio = static_cast<float>(m_imageSize.x) / m_imageSize.y;
        m_viewportSize = vec2(m_aspectRatio, 1) * 2.0f;
        m_pixelDelta = m_viewportSize / vec2(m_imageSize) * vec2(1, -1);
        m_pixelGridOrigin = m_cameraCenter - vec3(0, 0, m_focalLength) + vec3(-m_viewportSize.x, m_viewportSize.y, 0) / 2.0f + vec3(m_pixelDelta / 2.0f, 0);
    }

    vec3 rayColor(const Ray& ray, int bounces, const IHittable& world) const {
        if (bounces < 0)
            return vec3(0);

        auto hit = world.hit(ray, {0.001f, std::numeric_limits<float>::infinity()});
        if (hit.hit) {
            Ray scattered;
            vec3 attenuation;
            if (!hit.material->scatter(ray, hit, attenuation, scattered))
                return vec3(0);

            return attenuation * rayColor(scattered, bounces - 1, world);
        }

        // background
        auto unitDirection = normalize(ray.direction());
        return glm::mix(vec3(1.0f), vec3(0.5f, 0.7f, 1.0f), 0.5f * (unitDirection.y + 1.0f));
    }

    glm::u8vec3 rgbfromAccumulator(const vec3& color) const {
        auto output = color / static_cast<float>(m_samples);  // average samples
        output = glm::pow(output, vec3(1.0f / m_gamma));      // gamma correction
        output = glm::clamp(output, vec3(0), vec3(1));        // clamp to [0, 1]
        return glm::u8vec3(output * 255.0f);                  // convert to 8-bit
    }
};
