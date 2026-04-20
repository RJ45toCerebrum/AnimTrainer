// Created by Tyler on 4/16/2026.
#pragma once

#include <common.h>
#include <ATMath.h>

START_NAMESPACE(ATRandom)

using vec3 = glm::vec3;

// RayLib GetRandomValue random number generation not thread safe
// Use this instead...
double randomDoubleInRange(double min, double max);
float randomFloatInRange(float min, float max);
vec3 randomUnitCubeVector();
// WARNING: this is not completely uniform; Should do that at some point...
vec3 randomOnUnitSphereVector();

END_NAMESPACE