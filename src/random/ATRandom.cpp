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

Vector3 randomUnitCubeVector()
{
    std::uniform_real_distribution<> dis(-1, 1);
    return Vector3(dis(gen), dis(gen), dis(gen));
}

Vector3 randomOnUnitSphereVector()
{
    std::uniform_real_distribution<> dis(-1, 1);
    const Vector3 rv(dis(gen), dis(gen), dis(gen));
    return Vector3Normalize(rv);
}


END_NAMESPACE