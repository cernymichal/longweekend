#pragma once

#include <chrono>
#include <random>

#include "Math.h"

/*
 * @brief Fast pseudo random number generator by Sebastiano Vigna
 */
struct SplitMix64 {
    uint64_t state;

    /*
     * @brief Constructs a new instance.
     * @param seed Initial state.
     */
    explicit constexpr SplitMix64(uint64_t seed) : state(seed) {}

    /*
     * @brief Constructs a new instance seeded by the current time.
     */
    explicit SplitMix64() : SplitMix64(std::chrono::system_clock::now().time_since_epoch().count()) {}

    /*
     * @brief Generates a random uint64 and updates the state.
     */
    constexpr inline uint64_t operator()() {
        // https://prng.di.unimi.it/splitmix64.c

        state += 0x9e3779b97f4a7c15Ui64;

        uint64_t z = state;
        z ^= z >> 30;
        z *= 0xbf58476d1ce4e5b9Ui64;
        z ^= z >> 27;
        z *= 0x94d049bb133111ebUi64;
        z ^= z >> 31;

        return z;
    }

    static inline constexpr uint64_t min() {
        return 0;
    }

    static inline constexpr uint64_t max() {
        return uint64_t(-1);
    }
};

/*
 * @brief Very fast solid pseudo random number generator by David Blackman and Sebastiano Vigna
 *
 * https://prng.di.unimi.it/
 */
struct Xoshiro256SS {
    uint64_t state[4];

    /*
     * @brief Constructs a new instance.
     * @param seed 64b seed from which the initial state is generated using splitmix64.
     */
    explicit constexpr Xoshiro256SS(uint64_t seed) {
        SplitMix64 generator(seed);

        state[0] = generator();
        state[1] = generator();
        state[2] = generator();
        state[3] = generator();
    }

    /*
     * @brief Constructs a new instance seeded by the current time.
     */
    explicit Xoshiro256SS() : Xoshiro256SS(std::chrono::system_clock::now().time_since_epoch().count()) {}

    /*
     * @brief Generates a random uint64 and updates the state.
     */
    constexpr inline uint64_t operator()() {
        // https://prng.di.unimi.it/xoshiro256starstar.c

        uint64_t result = bitRotateLeft(state[1] * 5, 7) * 9;

        uint64_t t = state[1] << 17;
        state[2] ^= state[0];
        state[3] ^= state[1];
        state[1] ^= state[2];
        state[0] ^= state[3];
        state[2] ^= t;
        state[3] = bitRotateLeft(state[3], 45);

        return result;
    }

    static inline constexpr uint64_t min() {
        return 0;
    }

    static inline constexpr uint64_t max() {
        return uint64_t(-1);
    }

private:
    /*
     * @return value rotated by k places left.
     */
    static inline constexpr uint64_t bitRotateLeft(uint64_t value, int k) {
        return (value << k) | (value >> (64 - k));
    }
};

inline thread_local Xoshiro256SS RANDOM_GENERATOR;

/*
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 * @return A random T in the range [min, max).
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
template <typename T>
inline T random(T min, T max) {
    if constexpr (std::is_floating_point_v<T>)
        return min + (max - min) * static_cast<T>(RANDOM_GENERATOR()) / static_cast<T>(RANDOM_GENERATOR.max());
    else
        return min + RANDOM_GENERATOR() % (max - min);
}

/*
 * @return A random value of T or a floating-point number in the range [0, 1).
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
template <typename T>
inline T random() {
    if constexpr (std::is_floating_point_v<T>)
        return random<T>(0, 1);
    else if constexpr (std::is_same_v<T, bool>)
        return RANDOM_GENERATOR() & 1;
    else {
        auto value = RANDOM_GENERATOR();
        return *reinterpret_cast<T*>(&value);
    }
}

// Random vectors

/*
 * @return A random vector with values in the range in the range [min, max).
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
template <int L, typename T = float>
inline glm::vec<L, T> randomVec(const glm::vec<L, T>& min, const glm::vec<L, T>& max) {
    glm::vec<L, T> result;
    for (int i = 0; i < L; i++)
        result[i] = random<T>(min[i], max[i]);

    return result;
}

/*
 * @return A random vector with values in the range in the range [0, 1).
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
template <int L, typename T = float>
inline glm::vec<L, T> randomVec() {
    return randomVec<L, T>(glm::vec<L, T>(0), glm::vec<L, T>(1));
}

/*
 * @return A random unit vec3.
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
inline vec3 randomUnitVec3() {
    // TODO faster?

    while (true) {
        vec3 v = randomVec<3>(vec3(-1.0f), vec3(1.0f));
        if (glm::length2(v) <= 1.0f)
            return glm::normalize(v);
    }
}

/*
 * @return A random unit vec3 on a hemisphere given by a normal.
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
inline vec3 randomVecOnHemisphere(const vec3& normal) {
    auto v = randomUnitVec3();
    return glm::dot(v, normal) >= 0.0f ? v : -v;
}
