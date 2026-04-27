// Created by Tyler on 4/15/2026.
#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <array>
#include <tuple>
#include <optional>

#include "raylib.h"

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

    using Vec3Pack = std::tuple<glm::vec3,glm::vec3,glm::vec3>;

    constexpr glm::vec3 kVec3Zero = glm::zero<glm::vec3>();
    constexpr glm::vec3 kVec3One = glm::one<glm::vec3>();
    constexpr glm::vec3 kVec3Right{1,0,0};
    constexpr glm::vec3 kVec3Up{0,1,0};
    constexpr glm::vec3 kVec3Forward{0,0,1};

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
        glm::vec4 _position = glm::vec4(0,0,0,1);
        glm::quat _rotation = glm::identity<glm::quat>();

    public:
        Transform() = default;
        Transform(const glm::mat4& matrix);
        Transform(const glm::quat& rotation);
        Transform(const glm::vec3& position, const glm::quat& rotation);
        Transform(const glm::vec3& position);
        Transform(const Transform& other) = default;
        Transform& operator=(const Transform& other) = default;


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
        [[nodiscard]] Vec3Pack extractAxes() const;

        [[nodiscard]] glm::vec3 transformPosition(const glm::vec3& position) const;
        [[nodiscard]] glm::vec3 inverseTransformPosition(const glm::vec3& position) const;
        [[nodiscard]] glm::vec3 transformDirection(const glm::vec3& direction) const;
        [[nodiscard]] glm::vec3 inverseTransformDirection(const glm::vec3& direction) const;

        friend bool operator==(const Transform& left, const Transform& right);
        friend bool operator!=(const Transform& left, const Transform& right);
        friend Transform operator*(const Transform& left, const Transform& right);
        friend Transform operator*(const glm::quat& leftQuat, const Transform& right);
        friend Transform operator*(const Transform& left, const glm::quat& rightQuat);
        friend Transform operator^(const Transform& left, const Transform& right);
    };

    struct Ray
    {
        glm::vec3 origin;
        glm::vec3 direction;
    };

    Vector2 toRLVec(const glm::vec2& vec);
    Vector3 toRLVec(const glm::vec3& vec);
    Vector4 toRLVec(const glm::vec4& vec);

    Matrix glmToRayMatrix(const glm::mat4& matrix);
    Matrix glmToRayMatrix(const glm::quat& rotation, const glm::vec4& position);
    ::Ray rayToRLRay(const ATMath::Ray& ray);
    glm::quat rotation(float angleDegrees, const glm::vec3& axis);
    glm::mat4x4 rotationMatrix(float angleDegrees, const glm::vec3& axis);

    bool isNormalized(const glm::vec3& v);
    glm::vec3 perpendicular(const glm::vec3& v);
    float signedAngleBetween(const glm::vec3& from, const glm::vec3& to, const glm::vec3& axis);
    std::array<glm::vec3,8> getCubeCorners(const glm::vec3& hSize);
    ATMath::Ray transformRay(const Transform& transform, const Ray& ray);
    glm::vec3 evaluateRay(const Ray& ray, float param);
    // the returned glm::vec2 are the params. You need to evaluate the ray to get the actual points.
    // This allows you to get the
    std::optional<glm::vec2> raySphereIntersection(const ATMath::Ray& ray, const glm::vec3& sphereCenter, float radius);
    /// will only yield proper results of ray dir is normalized...
    glm::vec3 closestPointAlongRay(const Ray& ray, const glm::vec3& point);
    /// returns the 2 parameters of the rays such that
    /// (ray0.origin * ret.x * ray0.direction) is the closest point on ray0.
    /// (ray1.origin * ret.y * ray1.direction) will be the closest on ray1.
    /// The points that minimize the distance between rays...
    glm::vec2 getClosestPointsParams(const Ray& ray0, const Ray& ray1);
    /// similar to getClosestPointsParams
    /// but instead of returns both vec3's evaluated
    std::pair<glm::vec3, glm::vec3> getClosestPointsVectors(const Ray& ray0, const Ray& ray1);
    RayCollision getRayCollisionOBB(const Ray& worldRay, const Transform& transform, const glm::vec3& size);
} // end ATMath

