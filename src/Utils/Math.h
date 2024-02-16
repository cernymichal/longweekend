#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <ostream>

constexpr double PI = glm::pi<double>();
constexpr double TWO_PI = glm::two_pi<double>();
constexpr double HALF_PI = glm::half_pi<double>();

using vec2 = glm::vec2;
using dvec2 = glm::dvec2;
using ivec2 = glm::ivec2;
using uvec2 = glm::uvec2;
using vec3 = glm::vec3;
using dvec3 = glm::dvec3;
using ivec3 = glm::ivec3;
using uvec3 = glm::uvec3;
using vec4 = glm::vec4;
using dvec4 = glm::dvec4;
using ivec4 = glm::ivec4;
using uvec4 = glm::uvec4;
using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;
using quat = glm::quat;

constexpr inline vec3 reflect(const vec3& v, const vec3& normal) {
    return v - 2.0f * glm::dot(v, normal) * normal;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const glm::vec<2, T>& v) {
    os << "(" << v.x << ", " << v.y << ")";
    return os;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const glm::vec<3, T>& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const glm::vec<4, T>& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
    return os;
}

// A closed interval [min, max]
template <typename T>
struct Interval {
    T min, max;

    constexpr Interval() = default;

    constexpr Interval(T min, T max) : min(min), max(max) {}

    constexpr T length() const { return max - min; }

    constexpr bool contains(T value) const { return value >= min && value <= max; }

    constexpr bool surrounds(T value) const { return value > min && value < max; }

    constexpr T clamp(T value) const { return std::max(min, std::min(max, value)); }

    static const Interval<T> empty;
    static const Interval<T> universe;
};

template <typename T>
const inline Interval<T> Interval<T>::empty = {std::numeric_limits<T>::max(), std::numeric_limits<T>::lowest()};

template <typename T>
const inline Interval<T> Interval<T>::universe = {std::numeric_limits<T>::lowest(), std::numeric_limits<T>::max()};
