#pragma once

template <typename T>
class Texture {
public:
    bool m_gammaCorrected = false;

    Texture() = default;

    Texture(const uvec2& size, T*&& data = nullptr, bool gammaCorrected = false) : m_size(size), m_data(data), m_gammaCorrected(gammaCorrected) {
        if (!m_data)
            m_data = new T[m_size.x * m_size.y];
    }

    Texture(const uvec2& size, const T& value, bool gammaCorrected = false) : m_size(size), m_data(new T[size.x * size.y]), m_gammaCorrected(gammaCorrected) {
        clear(value);
    }

    Texture(const Texture& other) {
        // TODO copy on write?

        m_size = other.m_size;
        m_channels = other.m_channels;
        m_data = new T[m_size.x * m_size.y];
        m_gammaCorrected = other.m_gammaCorrected;

        for (size_t i = 0; i < m_size.x * m_size.y; i++)
            m_data[i] = other.m_data[i];
    }

    Texture(Texture&& other) noexcept {
        m_size = other.m_size;
        m_data = other.m_data;
        m_channels = other.m_channels;
        m_gammaCorrected = other.m_gammaCorrected;

        other.m_data = nullptr;
    }

    Texture& operator=(const Texture& other) = delete;

    Texture& operator=(Texture&& other) = delete;

    ~Texture() {
        if (!m_data)
            return;

        delete[] m_data;
    }

    // Bilinear interpolation
    const T sampleInterpolated(const vec2& uv) const {
        vec2 sampleUV = uv * vec2(m_size - uvec2(1));
        f32 tx = sampleUV.x - floor(sampleUV.x);
        f32 ty = sampleUV.y - floor(sampleUV.y);
        sampleUV = glm::floor(sampleUV);

        bool txIsWhole = tx < glm::epsilon<f32>() || (1 - tx) < glm::epsilon<f32>();
        bool tyIsWhole = ty < glm::epsilon<f32>() || (1 - ty) < glm::epsilon<f32>();

        if (txIsWhole && tyIsWhole) {
            // No interpolation needed
            sampleUV += glm::round(vec2(tx, ty));
            return sample(uvec2(sampleUV));
        }

        if (txIsWhole) {
            // Interpolate only in y
            sampleUV.x += round(tx);  // round(tx) is either 0 or 1
            return (1 - ty) * sample(uvec2(sampleUV.x, sampleUV.y)) + ty * sample(uvec2(sampleUV.x, sampleUV.y + 1));
        }

        if (tyIsWhole) {
            // Interpolate only in x
            sampleUV.y += round(ty);
            return (1 - tx) * sample(uvec2((sampleUV.x), sampleUV.y)) + ty * sample(uvec2(sampleUV.x + 1, sampleUV.y));
        }

        T x0 = (1 - tx) * sample(uvec2(sampleUV.x, sampleUV.y)) + tx * sample(uvec2(sampleUV.x + 1, sampleUV.y));
        T x1 = (1 - tx) * sample(uvec2(sampleUV.x, sampleUV.y + 1)) + tx * sample(uvec2(sampleUV.x + 1, sampleUV.y + 1));

        return x0 * (1 - ty) + x1 * ty;
    }

    inline const T& sample(const vec2& uv) const {
        vec2 sampleUV = uv * vec2(m_size - uvec2(1));
        return sample(uvec2(sampleUV));
    }

    inline const T& sample(const uvec2& uv) const {
        uvec2 sampleUV = repeatUV(uv);
        return m_data[sampleUV.y * m_size.x + sampleUV.x];
    }

    inline T& sample(const uvec2& uv) {
        uvec2 sampleUV = repeatUV(uv);
        return m_data[sampleUV.y * m_size.x + sampleUV.x];
    }

    inline const T& operator[](const uvec2& uv) const { return sample(uv); }

    inline T& operator[](const uvec2& uv) { return sample(uv); }

    inline void clear(const T& value) {
        for (size_t i = 0; i < m_size.x * m_size.y; i++)
            m_data[i] = value;
    }

    inline const uvec2& size() const { return m_size; }

    inline const u8& channels() const { return m_channels; }

    inline const T* data() const { return m_data; }

private:
    uvec2 m_size = uvec2(0);
    u8 m_channels = 0;
    T* m_data = nullptr;

    inline uvec2 repeatUV(const uvec2& uv) const {
        return uv % m_size;
    }
};
