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

inline Xoshiro256SS RANDOM_GENERATOR;

// TODO templates

/*
 * @return A random uint64.
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
inline uint64_t randomUInt64() {
    return RANDOM_GENERATOR();
}

/*
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 * @return A random int32 in the range [min, max).
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
inline int32_t randomInt(int32_t min, int32_t max) {
    std::uniform_int_distribution<int32_t> distribution(min, max);
    return distribution(RANDOM_GENERATOR);
}

/*
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 * @return A random float in the range [min, max).
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
inline float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(RANDOM_GENERATOR);
}

/*
 * @return A random float in the range [0, 1).
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
inline float randomFloat() {
    return randomFloat(0.0f, 1.0f);
}

/*
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 * @return A random double in the range [min, max).
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
inline double randomDouble(double min, double max) {
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(RANDOM_GENERATOR);
}

/*
 * @return A random double in the range [0, 1).
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
inline double randomDouble() {
    return randomDouble(0.0, 1.0);
}

/*
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 * @return A random vec3 with components in the range [min, max).
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
inline vec3 randomVec3(float min, float max) {
    return vec3(randomFloat(min, max), randomFloat(min, max), randomFloat(min, max));
}

/*
 * @return A random vec3 with components in the range [0, 1).
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
inline vec3 randomVec3() {
    return randomVec3(0.0f, 1.0f);
}

/*
 * @return A random vec3 contained in a unit sphere.
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
inline vec3 randomVecInUnitSphere() {
    // TODO make faster?

    while (true) {
        vec3 v = randomVec3(-1.0f, 1.0f);
        if (glm::length2(v) > 1.0f)
            continue;

        return v;
    }
}

/*
 * @return A random unit vec3.
 *
 * @note Uses RANDOM_GENERATOR internally.
 */
inline vec3 randomUnitVec3() {
    return glm::normalize(randomVecInUnitSphere());
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
