// Created by Tyler on 4/15/2026.
#include "ATMath.h"
#include "raymath.h"

std::array<Vector3,8> ATMath::getCubeCorners(const Vector3& hSize)
{
    return {
        Vector3{-hSize.x, -hSize.y , -hSize.z}, Vector3{hSize.x, -hSize.y, -hSize.z}, Vector3{hSize.x, hSize.y, -hSize.z}, Vector3{-hSize.x, hSize.y, -hSize.z},
        Vector3{-hSize.x, -hSize.y,  hSize.z}, Vector3{hSize.x, -hSize.y,  hSize.z}, Vector3{hSize.x, hSize.y,  hSize.z}, Vector3{-hSize.x, hSize.y,  hSize.z}
    };
}


float ATMath::getPlaneRayIntersection(const Ray& ray, const Vector3& planePoint, const Vector3& planeNormal)
{
    const float normalPointDot = Vector3DotProduct(planeNormal, planePoint);
    const float normalRayDot = Vector3DotProduct(planeNormal, ray.position);
    const float normalRayDirDot = Vector3DotProduct(planeNormal, ray.direction);
    return (normalPointDot - normalRayDot) / normalRayDirDot;
}

Vector3 ATMath::evaluateRay(const Ray& ray, const float param)
{
    return ray.position + ray.direction * param;
}

Vector3 ATMath::closestPointAlongRay(const Ray& ray, const Vector3& point)
{
    const Vector3 toRayPos = ray.position - point;
    const float parallel = Vector3DotProduct(toRayPos, ray.direction);
    return point + toRayPos - ray.direction * parallel;
}

Vector2 ATMath::getClosestPointsParams(const Ray& ray0, const Ray& ray1)
{
    const float a = Vector3DotProduct(ray0.direction, ray0.direction);
    const float b = Vector3DotProduct(ray0.direction, ray1.direction);
    const float c = Vector3DotProduct(ray1.direction, ray1.direction);
    // Vector pointing from ray0 origin to ray1 origin
    const Vector3 w = ray1.position - ray0.position;
    const float d = Vector3DotProduct(w, ray0.direction);
    const float e = Vector3DotProduct(w, ray1.direction);
    const float denom = a * c - b * b;
    if (denom < 1e-6f)
        return {0.0f, 0.0f};
    return {
        (d * c - e * b) / denom,
        (d * b - e * a) / denom
    };
}