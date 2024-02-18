#pragma once

#include "IHittable.h"
#include "Material.h"

class Camera {
public:
    glm::uvec2 m_imageSize = glm::uvec2(256, 256);
    unsigned m_samples = 4;
    unsigned m_maxBounces = 10;
    float m_gamma = 2.2f;

    float m_fov = 90.0f;
    vec3 m_position = vec3(0);
    vec3 m_lookAt = vec3(0, 0, -1);
    vec3 m_up = vec3(0, 1, 0);

    void render(const IHittable& world, std::vector<glm::u8vec3>& target) {
        initialize();
        LOG(std::format("rendering image {0}x{1}", m_imageSize.x, m_imageSize.y));

#pragma omp parallel for
        for (size_t y = 0; y < m_imageSize.y; y++) {
            // LOG(std::format("{:.1f}% done", y * 100.0 / m_imageSize.y));

            for (size_t x = 0; x < m_imageSize.x; x++) {
                auto pixelCenter = m_pixelGridOrigin + static_cast<float>(x) * m_pixelDeltaU + static_cast<float>(y) * m_pixelDeltaV;
                auto color = vec3(0);

                for (unsigned i = 0; i < m_samples; i++) {
                    auto jitter = randomVec<2>() - 0.5f;
                    auto pixelSamplePoint = pixelCenter + jitter.x * m_pixelDeltaU + jitter.y * m_pixelDeltaU;
                    Ray ray(m_position, pixelSamplePoint - m_position);

                    color += rayColor(ray, m_maxBounces, world);
                }

                target[y * m_imageSize.x + x] = rgbfromAccumulator(color);
            }
        }
        LOG("100% done");
    }

private:
    vec2 m_viewportSize = vec2(1);
    vec3 m_pixelDeltaU = vec3(0);
    vec3 m_pixelDeltaV = vec3(0);
    vec3 m_pixelGridOrigin = vec3(0);

    void initialize() {
        auto focalLength = 1.0f;
        auto aspectRatio = static_cast<float>(m_imageSize.x) / m_imageSize.y;
        m_viewportSize = vec2(aspectRatio, 1) * 2.0f * focalLength * glm::tan(glm::radians(m_fov / 2.0f));

        vec3 u, v, w;
        w = glm::normalize(m_position - m_lookAt);
        u = glm::normalize(glm::cross(m_up, w));
        v = glm::cross(w, u);

        vec3 viewportU, viewportV;
        viewportU = u * m_viewportSize.x;
        viewportV = -v * m_viewportSize.y;
        m_pixelDeltaU = viewportU / static_cast<float>(m_imageSize.x);
        m_pixelDeltaV = viewportV / static_cast<float>(m_imageSize.y);
        m_pixelGridOrigin = m_position - focalLength * w - viewportU / 2.0f - viewportV / 2.0f + m_pixelDeltaU / 2.0f + m_pixelDeltaV / 2.0f;
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
