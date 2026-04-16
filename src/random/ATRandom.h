// Created by Tyler on 4/16/2026.
#pragma once

#include <common.h>
#include "raymath.h"

START_NAMESPACE(ATRandom)

Vector3 randomPointInBox(const Vector3& boxCenter, const Vector3& hsize);
// RayLib GetRandomValue random number generation not thread safe
// Use this instead...
double randomDoubleInRange(double min, double max);
Vector3 randomUnitCubeVector();

END_NAMESPACE