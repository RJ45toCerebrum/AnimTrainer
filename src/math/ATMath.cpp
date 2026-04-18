// Created by Tyler on 4/15/2026.
#include "ATMath.h"
//#include "raymath.h"
#include "glm/gtx/intersect.hpp"
#include "glm/gtx/quaternion.hpp"
#include <glm/gtx/dual_quaternion.hpp>


using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::quat;
using glm::mat4x4;
// just to make it clear that Ray Lib defines Ray;
// but ATMath defines a Ray as well so we can be compatible with glm types.
using RLRay = Ray;
using ATRay = ATMath::Ray;
using RLMatrix = Matrix;
using ATTransform = ATMath::Transform;


Vector2 ATMath::toRLVec(const vec2& vec)
{
    return {vec.x, vec.y};
}
Vector3 ATMath::toRLVec(const vec3& vec)
{
    return {vec.x, vec.y, vec.z};
}
Vector4 ATMath::toRLVec(const vec4& vec)
{
    return {vec.x, vec.y, vec.z, vec.w};
}

quat ATMath::rotation(float angleDegrees, const vec3& axis)
{
    const float angle = glm::radians(angleDegrees);
    return glm::angleAxis(angle, axis);
}
mat4x4 ATMath::rotationMatrix(const float angleDegrees, const vec3& axis)
{
    const float angle = glm::radians(angleDegrees);
    return glm::rotate(mat4x4(1.0f), angle, axis);
}

ATTransform::Transform(const mat4x4& matrix) :
    _position(vec3(matrix[3]), 1.0f), _rotation(glm::quat_cast(matrix))
{}
ATTransform::Transform(const quat& rotation) :
    _position(0,0,0,1), _rotation(rotation)
{}
ATTransform::Transform(const vec3& position, const quat& rotation) :
    _position(position, 1.0f), _rotation(rotation)
{}
ATTransform::Transform(const vec3& position) :
    _position(position, 1.0f)
{}

mat4x4 ATTransform::toMatrix() const
{
    mat4x4 m = glm::mat4_cast(_rotation);
    m[3] = vec4(_position.x, _position.y, _position.z, 1.0f);
    return m;
}
RLMatrix ATTransform::toRayMatrix() const
{
    const mat4x4 glmMat = toMatrix();
    return glmToRayMatrix(glmMat);
}

ATTransform ATTransform::inverse() const
{
    Transform inv;
    inv._rotation = glm::conjugate(_rotation);
    const vec3 pos(_position.x, _position.y, _position.z);
    inv._position = vec4(inv._rotation * -pos, 1.0f);
    return inv;
}

void ATTransform::invert()
{
    const quat invRotation = glm::conjugate(_rotation);
    const vec3 invPos = invRotation * -vec3(_position);
    _position = vec4(invPos, 1.0f);
    _rotation = invRotation;
}

void ATTransform::translate(const vec3& v)
{
    _position += vec4(v, 0.0f);
}

vec3 ATTransform::getPosition() const
{
    return {_position.x, _position.y, _position.z};
}

void ATTransform::setPosition(const vec3& point)
{
    _position = vec4(point, 1.0f);
}

quat ATTransform::getRotation() const
{
    return _rotation;
}

void ATTransform::setRotation(const quat& rotation)
{
    assert(glm::epsilonEqual(glm::length(rotation), 1.0f, glm::epsilon<float>()));
    _rotation = rotation;
}

ATMath::Vec3Pack ATTransform::extractAxes() const
{
    return {_rotation * kVec3Right, _rotation * kVec3Up, _rotation * kVec3Forward};
}

vec3 ATTransform::transformPosition(const vec3& position) const
{
    const auto dq = glm::dualquat(_rotation, _position);
    return dq * position;
}

vec3 ATTransform::inverseTransformPosition(const vec3& position) const
{
    const vec3 relativePos = position - vec3(_position);
    return glm::conjugate(_rotation) * relativePos;
}

vec3 ATTransform::transformDirection(const vec3& direction) const
{
    return _rotation * direction;
}

vec3 ATTransform::inverseTransformDirection(const vec3& direction) const
{
    return glm::inverse(_rotation) * direction;
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

ATTransform ATMath::operator*(const Transform& left, const Transform& right)
{
    Transform result;
    result._rotation = left._rotation * right._rotation;
    const vec3 rotatedRightPos = left._rotation * vec3(right._position);
    result._position = vec4(rotatedRightPos + vec3(left._position), 1.0f);
    return result;
}

ATTransform ATMath::operator^(const Transform& left, const Transform& right)
{
    return left.inverse() * right;
}

ATTransform ATMath::operator*(const quat& leftQuat, const Transform& right)
{
    Transform result;
    result._rotation = leftQuat * right._rotation;
    result._position = vec4(leftQuat * vec3(right._position), 1.0f);
    return result;
}

ATTransform ATMath::operator*(const Transform& left, const quat& rightQuat)
{
    Transform result;
    result._rotation = glm::normalize(left._rotation * rightQuat);
    result._position = left._position; // local rotation doesn't move the origin
    return result;
}


RLMatrix ATMath::glmToRayMatrix(const mat4x4& matrix)
{
    RLMatrix rayMatrix = {};
    rayMatrix.m0  = matrix[0][0]; rayMatrix.m4  = matrix[1][0]; rayMatrix.m8  = matrix[2][0]; rayMatrix.m12 = matrix[3][0];
    rayMatrix.m1  = matrix[0][1]; rayMatrix.m5  = matrix[1][1]; rayMatrix.m9  = matrix[2][1]; rayMatrix.m13 = matrix[3][1];
    rayMatrix.m2  = matrix[0][2]; rayMatrix.m6  = matrix[1][2]; rayMatrix.m10 = matrix[2][2]; rayMatrix.m14 = matrix[3][2];
    rayMatrix.m3  = matrix[0][3]; rayMatrix.m7  = matrix[1][3]; rayMatrix.m11 = matrix[2][3]; rayMatrix.m15 = matrix[3][3];
    return rayMatrix;
}

RLMatrix ATMath::glmToRayMatrix(const quat& rotation, const vec4& position)
{
    mat4x4 transform = glm::mat4_cast(rotation);
    transform[3] = position;
    return glmToRayMatrix(transform);
}

RLRay ATMath::rayToRLRay(const ATRay& ray)
{
    const Vector3 ori {ray.origin.x, ray.origin.y, ray.origin.z};
    const Vector3 dir {ray.direction.x, ray.direction.y, ray.direction.z};
    return {ori, dir};
}


bool ATMath::isNormalized(const vec3& v)
{
    constexpr auto epi = 10 * glm::epsilon<float>();
    const float absVal = std::abs(1 - glm::length2(v));
    return absVal < epi;
}

vec3 ATMath::perpendicular(const vec3& v)
{
    if (std::abs(v.x) < std::abs(v.y) && std::abs(v.x) < std::abs(v.z))
        return vec3{0, -v.z, v.y};
    if (std::abs(v.y) < std::abs(v.z))
        return vec3{-v.z, 0, v.x};
    return vec3{-v.y, v.x, 0};
}

std::array<vec3,8> ATMath::getCubeCorners(const vec3& hSize)
{
    return
    {
        vec3{-hSize.x, -hSize.y , -hSize.z}, vec3{hSize.x, -hSize.y, -hSize.z}, vec3{hSize.x, hSize.y, -hSize.z}, vec3{-hSize.x, hSize.y, -hSize.z},
        vec3{-hSize.x, -hSize.y,  hSize.z}, vec3{hSize.x, -hSize.y,  hSize.z}, vec3{hSize.x, hSize.y,  hSize.z}, vec3{-hSize.x, hSize.y,  hSize.z}
    };
}

std::optional<vec2> ATMath::raySphereIntersection(const ATMath::Ray& ray, const vec3& sphereCenter, const float radius)
{
    const vec3 L = ray.origin - sphereCenter;

    // Coefficients for: at^2 + bt + c = 0
    const float a = glm::dot(ray.direction, ray.direction);
    const float b = 2.0f * glm::dot(ray.direction, L);
    const float c = glm::dot(L, L) - (radius * radius);
    const float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0)
        return std::nullopt;

    // To find the closest hit point, solve for the smaller t
    const float sqrtDisc = std::sqrt(discriminant);
    const float t0 = (-b - sqrtDisc) / (2.0f * a);
    const float t1 = (-b + sqrtDisc) / (2.0f * a);
    const vec2 rv{std::min(t0, t1), std::max(t0,t1)};
    return std::optional<vec2>{rv};
}

vec3 ATMath::evaluateRay(const Ray& ray, const float param)
{
    return ray.origin + ray.direction * param;
}

ATRay ATMath::transformRay(const Transform& transform, const ATRay& ray)
{
    const vec3 tOrigin = transform.transformPosition(ray.origin);
    const vec3 tDirection = transform.transformDirection(ray.direction);
    return ATRay(tOrigin, tDirection);
}

vec3 ATMath::closestPointAlongRay(const Ray& ray, const vec3& point)
{
    assert(isNormalized(ray.direction));
    const vec3 toRayPos = ray.origin - point;
    const float parallel = glm::dot(toRayPos, ray.direction);
    return point + toRayPos - ray.direction * parallel;
}

vec2 ATMath::getClosestPointsParams(const Ray& ray0, const Ray& ray1)
{
    assert(isNormalized(ray0.direction));
    assert(isNormalized(ray1.direction));
    const float a = glm::dot(ray0.direction, ray0.direction);
    const float b = glm::dot(ray0.direction, ray1.direction);
    const float c = glm::dot(ray1.direction, ray1.direction);
    // Vector pointing from ray0 origin to ray1 origin
    const vec3 w = ray1.origin - ray0.origin;
    const float d = glm::dot(w, ray0.direction);
    const float e = glm::dot(w, ray1.direction);
    const float denom = a * c - b * b;
    if (glm::epsilonEqual(denom, 0.0f, 3 * glm::epsilon<float>()))
        return {0.0f, 0.0f};
    return {
        (d * c - e * b) / denom,
        (d * b - e * a) / denom
    };
}

std::pair<vec3, vec3> ATMath::getClosestPointsVectors(const Ray& ray0, const Ray& ray1)
{
    const vec2 paramsXAxis = getClosestPointsParams(ray0, ray1);
    return {evaluateRay(ray0, paramsXAxis.x), evaluateRay(ray1, paramsXAxis.y)};
}

RayCollision ATMath::getRayCollisionOBB(const ATRay& worldRay, const Transform& transform, const vec3& size)
{
    const BoundingBox localBox =
    {
        Vector3{-size.x, -size.y, -size.z},
        Vector3{size.x,  size.y,  size.z}
    };
    const Transform invTransform = transform.inverse();
    const ATRay localRay = transformRay(invTransform, worldRay);
    const RLRay rlRay = rayToRLRay(localRay);
    RayCollision collision = GetRayCollisionBox(rlRay, localBox);
    if (collision.hit)
    {
        // Note: 'distance' remains correct if your scale is uniform (1,1,1)
        // TODO: avoid so much conversion by making my own RayCollision struct.
        const vec3 worldCollisionPoint =
            transform.transformPosition(vec3{collision.point.x, collision.point.y, collision.point.z});
        collision.point = Vector3{worldCollisionPoint.x, worldCollisionPoint.y, worldCollisionPoint.z};
    }
    return collision;
}
