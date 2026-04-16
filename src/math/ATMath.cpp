// Created by Tyler on 4/15/2026.
#include "ATMath.h"

std::array<Vector3,8> ATMath::getCubeCorners(const Vector3& hSize)
{
    return {
        Vector3{-hSize.x, -hSize.y , -hSize.z}, Vector3{hSize.x, -hSize.y, -hSize.z}, Vector3{hSize.x, hSize.y, -hSize.z}, Vector3{-hSize.x, hSize.y, -hSize.z},
        Vector3{-hSize.x, -hSize.y,  hSize.z}, Vector3{hSize.x, -hSize.y,  hSize.z}, Vector3{hSize.x, hSize.y,  hSize.z}, Vector3{-hSize.x, hSize.y,  hSize.z}
    };
}
