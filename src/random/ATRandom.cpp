// Created by Tyler on 4/16/2026.
#include "ATRandom.h"
#include <random>

START_NAMESPACE(ATRandom)

// make sure only internal linkage with static
static thread_local std::random_device rd;
static thread_local std::mt19937 gen(rd());


double randomDoubleInRange(double min, double max)
{
    if (min > max)
        std::swap(min, max);
    std::uniform_real_distribution<> dis(min, max);
    return dis(gen);
}

float randomFloatInRange(float min, float max)
{
    if (min > max)
        std::swap(min, max);
    std::uniform_real_distribution<> dis(min, max);
    return dis(gen);
}

vec3 randomUnitCubeVector()
{
    std::uniform_real_distribution<> dis(-1, 1);
    return {dis(gen), dis(gen), dis(gen)};
}

vec3 randomOnUnitSphereVector()
{
    std::uniform_real_distribution<> dis(-1, 1);
    const vec3 rv {
        static_cast<float>(dis(gen)),
        static_cast<float>(dis(gen)),
        static_cast<float>(dis(gen))
    };
    return glm::normalize(rv);
}

END_NAMESPACE