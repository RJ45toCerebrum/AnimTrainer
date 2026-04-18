// Created by Tyler on 4/16/2026.
#pragma once

#include <common.h>
#include "raymath.h"

START_NAMESPACE(ATRandom)

// RayLib GetRandomValue random number generation not thread safe
// Use this instead...
double randomDoubleInRange(double min, double max);
Vector3 randomUnitCubeVector();

// WARNING: this is not completely uniform; Should do that at some point...
Vector3 randomOnUnitSphereVector();

END_NAMESPACE