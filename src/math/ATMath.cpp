// Created by Tyler on 4/15/2026.
#include "ATMath.h"
#include "raymath.h"
#include "glm/gtx/quaternion.hpp"


glm::mat4x4 ATMath::Transform::toMatrix() const
{
    glm::mat4 mat = glm::mat4_cast(_rotation);
    mat[3] = _position;
    return mat;
}
Matrix ATMath::Transform::toRayMatrix() const
{
    const glm::mat4x4 matrixForm = toMatrix();
    Matrix rayMatrix;
    std::memcpy(&rayMatrix, &matrixForm, sizeof(Matrix));
    return rayMatrix;
}

ATMath::Transform ATMath::Transform::inverse() const
{
    Transform inv;
    inv._rotation = glm::conjugate(_rotation);
    const glm::vec3 pos(_position.x, _position.y, _position.z);
    inv._position = glm::vec4(inv._rotation * -pos, 1.0f);
    return inv;
}

void ATMath::Transform::invert()
{
    const glm::quat invRotation = glm::conjugate(_rotation);
    const glm::vec3 invPos = invRotation * -glm::vec3(_position);
    _position = glm::vec4(invPos, 1.0f);
    _rotation = invRotation;
}

void ATMath::Transform::translate(const glm::vec3& v)
{
    _position += glm::vec4(v, 0.0f);
}

glm::vec3 ATMath::Transform::getPosition() const
{
    return {_position.x, _position.y, _position.z};
}

void ATMath::Transform::setPosition(const glm::vec3& point)
{
    _position = glm::vec4(point, 1.0f);
}

glm::quat ATMath::Transform::getRotation() const
{
    return _rotation;
}

void ATMath::Transform::setRotation(const glm::quat& rotation)
{
    assert(glm::epsilonEqual(glm::length2(rotation), 1.0f, glm::epsilon<float>()));
    _rotation = rotation;
}

bool ATMath::operator==(const Transform& left, const Transform& right)
{
    const bool positionsEqual =
        glm::all(glm::epsilonEqual(left._position, right._position, glm::epsilon<float>()));
    const bool rotationsEqual =
        glm::all(glm::epsilonEqual(left._rotation, right._rotation, glm::epsilon<float>()));
    return positionsEqual and rotationsEqual;
}

bool ATMath::operator!=(const Transform &left, const Transform &right)
{
    return !(left == right);
}

ATMath::Transform ATMath::operator*(const Transform& left, const Transform& right)
{
    Transform result;
    result._rotation = left._rotation * right._rotation;
    const glm::vec3 rotatedRightPos = left._rotation * glm::vec3(right._position);
    result._position = glm::vec4(rotatedRightPos + glm::vec3(left._position), 1.0f);
    return result;
}

ATMath::Transform ATMath::operator^(const Transform& left, const Transform& right)
{
    return left.inverse() * right;
}

Matrix ATMath::glmToRayMatrix(const glm::mat4& matrix)
{
    Matrix rayMatrix = {};
    std::memcpy(&rayMatrix, &matrix, sizeof(Matrix));
    return rayMatrix;
}

Matrix ATMath::glmToRayMatrix(const glm::quat& rotation, const glm::vec4& position)
{
    glm::mat4 transform = glm::mat4_cast(rotation);
    transform[3] = position;
    Matrix mat = {};
    std::memcpy(&mat, &transform, sizeof(Matrix));
    return mat;
}

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

RayCollision ATMath::getRayCollisionOBB(const Ray& worldRay, const Matrix& transform, const Vector3& size)
{
    const BoundingBox localBox =
    {
        Vector3{-size.x, -size.y, -size.z},
        Vector3{size.x,  size.y,  size.z}
    };

    Matrix invTransform = MatrixInvert(transform);

    Ray localRay;
    localRay.position = Vector3Transform(worldRay.position, invTransform);
    invTransform.m12 = invTransform.m13 = invTransform.m14 = 0.0f;
    localRay.direction = Vector3Transform(worldRay.direction, invTransform);

    RayCollision collision = GetRayCollisionBox(localRay, localBox);
    if (collision.hit) {
        // Note: 'distance' remains correct if your scale is uniform (1,1,1)
        collision.point = Vector3Transform(collision.point, transform);
    }
    return collision;
}
