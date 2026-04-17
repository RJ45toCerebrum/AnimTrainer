// Created by Tyler on 4/15/2026.
#pragma once

#include <concepts>
#include <array>

#include "raylib.h"

namespace ATMath
{
    constexpr static Vector3 VZERO{0,0,0};
    constexpr static Vector3 VONE{1,1,1};
    constexpr static Vector3 RIGHT{1,0,0};
    constexpr static Vector3 UP{0,1,0};
    constexpr static Vector3 FORWARD{0,0,1};

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

    enum class Axis
    {
        None,
        X,
        Y,
        Z,
        W,
    };

    std::array<Vector3,8> getCubeCorners(const Vector3& hSize);
    float getPlaneRayIntersection(const Ray& ray, const Vector3& planePoint, const Vector3& planeNormal);
    Vector3 evaluateRay(const Ray& ray, float param);

    // will only yield proper results of ray dir is is normalized...
    Vector3 closestPointAlongRay(const Ray& ray, const Vector3& point);
    Vector2 getClosestPointsParams(const Ray& ray0, const Ray& ray1);
    RayCollision getRayCollisionOBB(const Ray& worldRay, const Matrix& transform, const Vector3& size);
} // end ATMath

