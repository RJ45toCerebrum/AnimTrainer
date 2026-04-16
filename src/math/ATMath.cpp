// Created by Tyler on 4/15/2026.
#include "ATMath.h"

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