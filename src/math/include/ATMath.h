// Created by Tyler on 4/15/2026.
#pragma once
#include <concepts>

namespace ATMath
{
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

    struct Vector3D
    {
        int x = 0;
        int y = 0;
        int z = 0;
    };

    struct Vector4D
    {
        int x = 0;
        int y = 0;
        int z = 0;
        int w = 0;
    };

    // Will templatize later...
    Vector3D norm(const Vector3D& v);
    Vector4D norm(const Vector4D& v);
    double dot(const Vector3D& a, const Vector3D& b);
    double dot(const Vector4D& v, const Vector3D& w);
} // end ATMath

