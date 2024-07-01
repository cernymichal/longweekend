#pragma once

#include "Hittables/IHittable.h"
#include "Material.h"
#include "Texture.h"

class Camera {
public:
    glm::uvec2 m_imageSize = glm::uvec2(256, 256);
    u32 m_samples = 4;
    u32 m_maxBounces = 10;

    f32 m_fov = 90.0f;
    vec3 m_position = vec3(0);
    vec3 m_lookAt = VEC_FORWARD;  // TODO add transform to camera
    vec3 m_up = VEC_UP;

    f32 m_defocusAngle = 0.0f;
    f32 m_focusDistance = 10.0f;

    Ref<Texture<vec3>> m_environment;

    Texture<vec3> render(const IHittable& world, std::function<void(const Texture<vec3>&, u32)> sampleFinishCallback) {
        initialize();

        // Accumulate samples
        Texture<vec3> accumulator(m_imageSize);
        for (u32 sample = 0; sample < m_samples; sample++) {
            vec2 jitter = randomVec<2>() - 0.5f;
            vec3 jitteredSamplePoint = jitter.x * m_pixelDeltaU + jitter.y * m_pixelDeltaU;

            // Sample the whole frame
            NODEBUG(_Pragma("omp parallel for"))
            for (u32 y = 0; y < m_imageSize.y; y++) {
                for (u32 x = 0; x < m_imageSize.x; x++) {
                    vec3 pixelCenter = m_pixelGridOrigin + static_cast<f32>(x) * m_pixelDeltaU + static_cast<f32>(y) * m_pixelDeltaV;
                    vec3 pixelSamplePoint = pixelCenter + jitteredSamplePoint;
                    vec3 rayOrigin = m_defocusAngle > 0 ? randomInDefocusDisk() : m_position;
                    Ray ray(rayOrigin, pixelSamplePoint - rayOrigin);

                    accumulator[uvec2(x, y)] += rayColor(ray, world);
                }
            }

            sampleFinishCallback(accumulator, sample + 1);
        }

        // Average the samples
        for (u32 y = 0; y < m_imageSize.y; y++)
            for (u32 x = 0; x < m_imageSize.x; x++)
                accumulator[uvec2(x, y)] /= m_samples;

        return accumulator;
    }

private:
    vec2 m_viewportSize = vec2(1);
    vec3 m_pixelDeltaU = vec3(0);
    vec3 m_pixelDeltaV = vec3(0);
    vec3 m_pixelGridOrigin = vec3(0);
    vec3 m_defocusDiskU = vec3(0);
    vec3 m_defocusDiskV = vec3(0);

    void initialize() {
        auto aspectRatio = static_cast<f32>(m_imageSize.x) / m_imageSize.y;
        m_viewportSize = vec2(aspectRatio, 1) * 2.0f * m_focusDistance * glm::tan(glm::radians(m_fov / 2.0f));

        vec3 u, v, w;
        w = glm::normalize(m_position - m_lookAt);
        u = glm::normalize(glm::cross(m_up, w));
        v = glm::cross(w, u);

        vec3 viewportU, viewportV;
        viewportU = u * m_viewportSize.x;
        viewportV = -v * m_viewportSize.y;
        m_pixelDeltaU = viewportU / static_cast<f32>(m_imageSize.x);
        m_pixelDeltaV = viewportV / static_cast<f32>(m_imageSize.y);
        m_pixelGridOrigin = m_position - m_focusDistance * w - viewportU / 2.0f - viewportV / 2.0f + m_pixelDeltaU / 2.0f + m_pixelDeltaV / 2.0f;

        auto defocusRadius = m_focusDistance * tan(glm::radians(m_defocusAngle / 2));
        m_defocusDiskU = defocusRadius * u;
        m_defocusDiskV = defocusRadius * v;
    }

    vec3 rayColor(Ray ray, const IHittable& world) const {
        vec3 attenuation = vec3(1);
        vec3 incomingLight = vec3(0);

        for (u32 i = 0; i <= m_maxBounces; i++) {
            auto hit = world.hit(ray, {0.001f, std::numeric_limits<f32>::infinity()});
            if (!hit.hit) {
                incomingLight += attenuation * sampleEnvironment(ray);
                break;  // No hit, return environment
            }

            Material::ScatterOutput scatterOutput = hit.material->scatter(ray, hit);
            ray = scatterOutput.scatteredRay;
            incomingLight += attenuation * scatterOutput.emission;
            attenuation *= scatterOutput.attenuation;

            if (!scatterOutput.didScatter)
                break;  // Absorbed

            // if (glm::length2(attenuation) < 0.001f)
            //     break;  // Early termination for small values, might break high exposure
        }

        return incomingLight;
    }

    vec3 sampleEnvironment(const Ray& ray) const {
        auto unitDirection = normalize(ray.direction);

        if (m_environment) {
            f32 u = atan2(unitDirection.z, unitDirection.x) / TWO_PI + 0.5f;
            f32 v = acos(unitDirection.y) / PI;
            return m_environment->sampleI({u, v});
        }

        // return glm::mix(vec3(1.0f), vec3(0.5f, 0.7f, 1.0f), 0.5f * (unitDirection.y + 1.0f)); // sky gradient

        return vec3(0.05f);
    }

    vec3 randomInDefocusDisk() const {
        auto r = randomVec2InUnitDisk();
        return m_position + m_defocusDiskU * r.x + m_defocusDiskV * r.y;
    }
};
