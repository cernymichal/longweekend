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

    const T sampleI(const vec2& uv) const {
        auto sampleUV = uv * vec2(m_size - uvec2(1));

        if (glm::all(glm::fract(sampleUV) <= vec2(0.001f)))
            return sample(uvec2(sampleUV));

        // Bilinear interpolation

        f32 tx = sampleUV.x - floor(sampleUV.x);
        f32 ty = sampleUV.y - floor(sampleUV.y);

        T x0 = (1 - tx) * sample(uvec2(floor(sampleUV.x), floor(sampleUV.y))) + tx * sample(uvec2(floor(sampleUV.x) + 1, floor(sampleUV.y)));
        T x1 = (1 - tx) * sample(uvec2(floor(sampleUV.x), floor(sampleUV.y) + 1)) + tx * sample(uvec2(floor(sampleUV.x) + 1, floor(sampleUV.y) + 1));

        return x0 * (1 - ty) + x1 * ty;
    }

    inline const T& sample(const uvec2& uv) const {
        return m_data[uv.y * m_size.x + uv.x];
    }

    inline T& sample(const uvec2& uv) {
        return m_data[uv.y * m_size.x + uv.x];
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
};
