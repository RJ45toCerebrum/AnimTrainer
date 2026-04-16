// Created by Tyler on 4/15/2026.
#pragma once
#include <concepts>
#include <array>

#include "raymath.h"

namespace ATMath
{
    constexpr static Vector3 VZERO{0,0,0};
    constexpr static Vector3 VONE{1,1,1};

    // will templatize later....
    template<typename T>
    concept Number = std::floating_point<T> || std::integral<T>;

    template<typename T>
    concept MathVector = requires(T v)
    {
        {v.x} -> Number;
        {v.y} -> Number;
        {v.z} -> Number;
    };

    std::array<Vector3,8> getCubeCorners(const Vector3& hSize);
} // end ATMath

