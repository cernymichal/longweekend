#pragma once

class Transform {
public:
    void setPosition(const vec3& position) {
        m_position = position;
        m_cacheValid = false;
    }

    void move(const vec3& offset) {
        setPosition(position() + offset);
    }

    void setRotation(const quat& rotation) {
        m_rotation = rotation;
        m_cacheValid = false;
    }

    void rotate(const quat& offset) {
        setRotation(offset * m_rotation);
    }

    void setScale(const vec3& scale) {
        m_scale = scale;
        m_cacheValid = false;
    }

    void scale(const vec3& amount) {
        setScale(m_scale * amount);
    }

    vec3 position() const {
        return m_position;
    }

    quat rotation() const {
        return m_rotation;
    }

    vec3 scale() const {
        return m_scale;
    }

    const mat4& modelMatrix() const {
        if (!m_cacheValid)
            recalculateCache();

        return m_modelMatrix;
    }

    const mat4& modelMatrixInverse() const {
        if (!m_cacheValid)
            recalculateCache();

        return m_modelMatrixInverse;
    }

    vec3 forward() const {
        return glm::normalize(vec3(modelMatrix() * vec4(VEC_FORWARD, 0.0f)));
    }

    vec3 up() const {
        return glm::normalize(vec3(modelMatrix() * vec4(VEC_UP, 0.0f)));
    }

    vec3 right() const {
        return glm::normalize(vec3(modelMatrix() * vec4(VEC_RIGHT, 0.0f)));
    }

private:
    vec3 m_position = vec3(0.0f, 0.0f, 0.0f);
    quat m_rotation = quat(vec3(0.0f, 0.0f, 0.0f));
    vec3 m_scale = vec3(1.0f, 1.0f, 1.0f);

    mutable mat4 m_modelMatrix = mat4(1.0);
    mutable mat4 m_modelMatrixInverse = mat4(1.0);
    mutable bool m_cacheValid = true;

    void recalculateCache() const {
        m_modelMatrix = mat4(1.0f);
        m_modelMatrix = glm::translate(m_modelMatrix, m_position);
        m_modelMatrix *= glm::mat4_cast(m_rotation);
        m_modelMatrix = glm::scale(m_modelMatrix, m_scale);
        m_modelMatrixInverse = glm::inverse(m_modelMatrix);
        m_cacheValid = true;
    }
};
