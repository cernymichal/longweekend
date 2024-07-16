#pragma once

#include "Ray.h"

class Renderer;

class Camera {
public:
    f32 m_fov = 90.0f;
    vec3 m_position = vec3(0);
    vec3 m_lookAt = VEC_FORWARD;  // TODO add transform to camera
    vec3 m_up = VEC_UP;

    f32 m_defocusAngle = 0.0f;
    f32 m_focusDistance = 10.0f;

    Ray createRay(const uvec2& pixel, const vec2& sample) const {
        vec3 pixelCenter = m_pixelGridOrigin + (f32)pixel.x * m_pixelDeltaU + (f32)pixel.y * m_pixelDeltaV;
        vec3 samplePoint = pixelCenter + sample.x * m_pixelDeltaU + sample.y * m_pixelDeltaV;
        vec3 rayOrigin = m_position;

        if (m_defocusAngle > 0) {
            // Random position in defocus disk
            vec2 random = randomVec2InUnitDisk();
            rayOrigin += m_defocusDiskU * random.x + m_defocusDiskV * random.y;
        }

        return Ray(rayOrigin, samplePoint - rayOrigin);
    }

private:
    vec2 m_viewportSize = vec2(1);
    vec3 m_pixelDeltaU = vec3(0);
    vec3 m_pixelDeltaV = vec3(0);
    vec3 m_pixelGridOrigin = vec3(0);
    vec3 m_defocusDiskU = vec3(0);
    vec3 m_defocusDiskV = vec3(0);

    friend class Renderer;

    void initialize(uvec2 imageSize) {
        auto aspectRatio = static_cast<f32>(imageSize.x) / imageSize.y;
        m_viewportSize = vec2(aspectRatio, 1) * 2.0f * m_focusDistance * glm::tan(glm::radians(m_fov / 2.0f));

        vec3 u, v, w;
        w = glm::normalize(m_position - m_lookAt);
        u = glm::normalize(glm::cross(m_up, w));
        v = glm::cross(w, u);

        vec3 viewportU, viewportV;
        viewportU = u * m_viewportSize.x;
        viewportV = -v * m_viewportSize.y;
        m_pixelDeltaU = viewportU / static_cast<f32>(imageSize.x);
        m_pixelDeltaV = viewportV / static_cast<f32>(imageSize.y);
        m_pixelGridOrigin = m_position - m_focusDistance * w - viewportU / 2.0f - viewportV / 2.0f + m_pixelDeltaU / 2.0f + m_pixelDeltaV / 2.0f;

        auto defocusRadius = m_focusDistance * tan(glm::radians(m_defocusAngle / 2));
        m_defocusDiskU = defocusRadius * u;
        m_defocusDiskV = defocusRadius * v;
    }
};
