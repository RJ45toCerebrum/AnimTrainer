// Created by Tyler on 4/15/2026.
#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <concepts>
#include <array>

#include "raylib.h"
#include "raymath.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ATMath
{
    static_assert(sizeof(glm::mat4) == sizeof(Matrix),
        "RayLib Matrix must be the same size as glm::mat4 as ATMath requires it.");

    constexpr static Vector3 VZERO{0,0,0};
    constexpr static Vector3 VONE{1,1,1};
    constexpr static Vector3 RIGHT{1,0,0};
    constexpr static Vector3 UP{0,1,0};
    constexpr static Vector3 FORWARD{0,0,1};

    enum class Axis
    {
        None,
        X,
        Y,
        Z,
        W,
    };

    // transforms in AnimTrainer only translation and rotation. This is to keep things much more simple.
    // Also, this gives a speed advantage to all inverse math.
    class Transform
    {
        // BE careful accessing this directly with
        glm::vec4 _position = glm::vec4(0,0,0,1);
        glm::quat _rotation = glm::identity<glm::quat>();

    public:
        [[nodiscard]] glm::mat4x4 toMatrix() const;
        // internally, all AnimTrainer math is done with GLM, but we need to convert those math entities to
        // the types raylib understands...
        [[nodiscard]] Matrix toRayMatrix() const;
        // return the inverses of the transform
        [[nodiscard]] Transform inverse() const;
        // inverse transform in-place
        void invert();
        void translate(const glm::vec3& v);
        [[nodiscard]] glm::vec3 getPosition() const;
        void setPosition(const glm::vec3& point);
        [[nodiscard]] glm::quat getRotation() const;
        void setRotation(const glm::quat& rotation);

        friend bool operator==(const Transform& left, const Transform& right);
        friend bool operator!=(const Transform& left, const Transform& right);
        friend Transform operator*(const Transform& left, const Transform& right);
        friend Transform operator^(const Transform& left, const Transform& right);
    };

    Matrix glmToRayMatrix(const glm::mat4& matrix);
    Matrix glmToRayMatrix(const glm::quat& rotation, const glm::vec4& position);

    std::array<Vector3,8> getCubeCorners(const Vector3& hSize);
    float getPlaneRayIntersection(const Ray& ray, const Vector3& planePoint, const Vector3& planeNormal);
    Vector3 evaluateRay(const Ray& ray, float param);

    // will only yield proper results of ray dir is is normalized...
    Vector3 closestPointAlongRay(const Ray& ray, const Vector3& point);
    Vector2 getClosestPointsParams(const Ray& ray0, const Ray& ray1);
    RayCollision getRayCollisionOBB(const Ray& worldRay, const Matrix& transform, const Vector3& size);
} // end ATMath

