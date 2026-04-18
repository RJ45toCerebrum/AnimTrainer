#include <iostream>
#include <numbers>
#include <raylib.h>

#include "ATCamera.h"
#include "ATMath.h"
#include "ATRandom.h"

#include "raymath.h"
#include "rlgl.h"
#include "glm/gtc/random.hpp"
#include "glm/gtx/quaternion.hpp"


namespace
{
    constexpr std::string_view TITLE = "AnimTrainer";
}


using ATTransform = ATMath::Transform;
using VecPack = ATMath::Vec3Pack;
using ATRay = ATMath::Ray;
using glm::vec2;
using glm::vec3;
using glm::quat;
using glm::mat4x4;
using ATMath::toRLVec;


class TransformGizmo
{
    ATTransform transform;
    // bounding box / extents
    vec3 hsize = glm::one<vec3>();
    // will delete at some point
    ATMath::Axis selectedAxis = ATMath::Axis::None;
    vec3 lastAxisPosition = ATMath::kVec3Zero;

    // very inefficient as we create new mesh for every gizmo.
    // but must fix later. Better than mem-leak I had before...
    Mesh _coneMesh = GenMeshCone(1.33f, 3, 15);
    Mesh _cylinderMesh = GenMeshCylinder(0.01, 1, 15);

public:
    ~TransformGizmo()
    {
        UnloadMesh(_coneMesh);
        UnloadMesh(_cylinderMesh);
    }

    [[nodiscard]] ATTransform GetCurrentTransform() const
    {
        return transform;
    }
    [[nodiscard]] vec3 GetPosition() const
    {
        return transform.getPosition();
    }
    void SetPosition(const vec3& p)
    {
        transform.setPosition(p);
    }
    [[nodiscard]] vec3 GetExtents() const
    {
        return this->hsize;
    }
    void SetExtents(const vec3& extents)
    {
        hsize = extents;
    }
    [[nodiscard]] quat GetRotation() const
    {
        return transform.getRotation();
    }
    void SetRotation(const quat& q)
    {
        transform.setRotation(q);
    }
    void SetSelected(const ATMath::Axis selAxis)
    {
        this->selectedAxis = selAxis;
    }
    [[nodiscard]] VecPack GetWorldAxes() const
    {
        return transform.extractAxes();
    }

    void Update(const ATCamera::CameraController& cameraController)
    {
        //TranslationUpdate(cameraController);
    }
    void DrawTransformGizmo(const Material& transformMat) const
    {
        constexpr Color upAxisColor{83, 189, 116, 255};
        constexpr Color selectedColor = PURPLE;

        // draw up arrow (yellow) //rgb(83, 189, 116)
        transformMat.maps[MATERIAL_MAP_DIFFUSE].color =
            selectedAxis == ATMath::Axis::Y ? selectedColor : upAxisColor;
        DrawArrowGizmo(transformMat, transform, _cylinderMesh, _coneMesh);

        // draw x-axis arrow (red)
        transformMat.maps[MATERIAL_MAP_DIFFUSE].color =
            selectedAxis == ATMath::Axis::X ? selectedColor : RED;
        quat rotation = ATMath::rotation(-90, ATMath::kVec3Forward);
        ATTransform rotationTransform;
        rotationTransform.setRotation(rotation);
        ATTransform redArrowTransform = transform * rotationTransform;
        DrawArrowGizmo(transformMat, redArrowTransform, _cylinderMesh, _coneMesh);

        // Draw z-axis (blue)
        transformMat.maps[MATERIAL_MAP_DIFFUSE].color =
            selectedAxis == ATMath::Axis::Z ? selectedColor : BLUE;
        rotation = ATMath::rotation(90, ATMath::kVec3Right);
        rotationTransform.setRotation(rotation);
        ATTransform blueArrowTransform = transform * rotationTransform;
        DrawArrowGizmo(transformMat, blueArrowTransform, _cylinderMesh, _coneMesh);
    }
    void DrawBB() const
    {
        std::array<vec3,8> cubeCorners = ATMath::getCubeCorners(hsize);
        // mostly needed to avoid needing to convert too many times below
        std::array<Vector3,8> cubeCornersTransformed = {};
        for (std::size_t i = 0; i < cubeCorners.size(); i++)
        {
            const vec3 transformedCC = transform.transformPosition(cubeCorners[i]);
            cubeCornersTransformed[i] = toRLVec(transformedCC);
        }

        // Draw the 12 edges using DrawLine3D
        // Back face
        DrawLine3D(cubeCornersTransformed[0], cubeCornersTransformed[1], ORANGE);
        DrawLine3D(cubeCornersTransformed[1], cubeCornersTransformed[2], ORANGE);
        DrawLine3D(cubeCornersTransformed[2], cubeCornersTransformed[3], ORANGE);
        DrawLine3D(cubeCornersTransformed[3], cubeCornersTransformed[0], ORANGE);
        // Front face
        DrawLine3D(cubeCornersTransformed[4], cubeCornersTransformed[5], ORANGE);
        DrawLine3D(cubeCornersTransformed[5], cubeCornersTransformed[6], ORANGE);
        DrawLine3D(cubeCornersTransformed[6], cubeCornersTransformed[7], ORANGE);
        DrawLine3D(cubeCornersTransformed[7], cubeCornersTransformed[4], ORANGE);

        // Connecting lines
        for (int i = 0; i < 4; i++)
            DrawLine3D(cubeCornersTransformed[i], cubeCornersTransformed[i + 4], ORANGE);
    }
    static void DrawRotationGizmo()
    {
        constexpr Color kXAxisColor{255,0,0, 255};
        constexpr Color kYAxisColor{0,255,0, 255};
        constexpr Color kZAxisColor{0,0,255, 255};
        constexpr vec3 zeroVector{0,0,0};
        constexpr vec3 right{1,0,0};
        constexpr vec3 up{0,1,0};
        constexpr vec3 forward{0,0, 1};
        //static vec3 randomAxis = ATRandom::randomOnUnitSphereVector();
        //static Vector3 rx = {randomAxis.x, randomAxis.y, randomAxis.z};
        DrawCircleAroundAxis(1, right, kXAxisColor);
        DrawCircleAroundAxis(1, up, kYAxisColor);
        DrawCircleAroundAxis(1, forward, kZAxisColor);
    }

private:
    void TranslationUpdate(const ATCamera::CameraController& cameraController)
    {
        const ATRay mRay = cameraController.GetWorldMouseRay();
        const RayCollision rayCol = ATMath::getRayCollisionOBB(mRay, transform, hsize);
        if (not rayCol.hit)
        {
            SetSelected(ATMath::Axis::None);
            return;
        }

        const auto [right, up, forward] = GetWorldAxes();
        const vec3 curGizmoPos = GetPosition();
        const ATRay xRay{curGizmoPos, right};
        const ATRay yRay{curGizmoPos, up};
        const ATRay zRay{curGizmoPos, forward};

        const auto [av, cpOnXRay] = ATMath::getClosestPointsVectors(mRay, xRay);
        vec3 diff = av - cpOnXRay;
        const float xLength = glm::dot(diff, diff);

        const auto [bv, cpOnYRay] = ATMath::getClosestPointsVectors(mRay, yRay);
        diff = bv - cpOnYRay;
        const float yLength = glm::dot(diff, diff);

        const auto [cv, cpOnZRay] = ATMath::getClosestPointsVectors(mRay, zRay);
        diff = cv - cpOnZRay;
        const float zLength = glm::dot(diff, diff);


        vec3 closestAxisPoint = glm::zero<vec3>();
        vec3 movementDirection = glm::zero<vec3>();
        const bool leftMouseDown = IsMouseButtonDown(MouseButton::MOUSE_BUTTON_LEFT);
        if (leftMouseDown and selectedAxis != ATMath::Axis::None)
        {
            // in this if block, if we hold the mouse down, we never allow switch
            // to different axis because it results in sudden jumps in the gizmo.
            if (selectedAxis == ATMath::Axis::X)
            {
                closestAxisPoint = cpOnXRay;
                movementDirection = right;
            }
            else if (selectedAxis == ATMath::Axis::Y)
            {
                closestAxisPoint = cpOnYRay;
                movementDirection = up;
            }
            else
            {
                closestAxisPoint = cpOnZRay;
                movementDirection = forward;
            }
        }
        else if (xLength < yLength)
        {
            if (zLength < xLength)
            {
                SetSelected(ATMath::Axis::Z);
                closestAxisPoint = cpOnZRay;
                movementDirection = forward;
            }
            else
            {
                SetSelected(ATMath::Axis::X);
                closestAxisPoint = cpOnXRay;
                movementDirection = right;
            }
        }
        else
        {
            if (zLength < yLength)
            {
                SetSelected(ATMath::Axis::Z);
                closestAxisPoint = cpOnZRay;
                movementDirection = forward;
            }
            else
            {
                SetSelected(ATMath::Axis::Y);
                closestAxisPoint = cpOnYRay;
                movementDirection = up;
            }
        }
        // below for debugging. Uncomment if needed.
        // DrawSphere(ATMath::toRLVec(closestAxisPoint), 0.03f, ORANGE);

        if (!leftMouseDown)
        {
            // too far from axis to select...
            if (std::ranges::min({xLength, zLength, yLength}) > 0.005f)
                SetSelected(ATMath::Axis::None);
        }
        else
        {
            const vec3 v = closestAxisPoint - lastAxisPosition;
            // movementDirection = the direction of the selected axis...
            // prevents jumping...
            const vec3 newGizmoPos = curGizmoPos + glm::dot(v, movementDirection) * movementDirection;
            SetPosition(newGizmoPos);
        }
        lastAxisPosition = closestAxisPoint;
    }

    void RotationUpdate(const ATCamera::CameraController& cameraController)
    {
        const ATRay mRay = cameraController.GetWorldMouseRay();
        const RayCollision rayCol = ATMath::getRayCollisionOBB(mRay, transform, hsize);
        if (not rayCol.hit)
            return;

        // for now. I just assume gizmo at center... MUST FIX
    }

    static void DrawArrowGizmo(const Material& arrowMaterial,
        const ATTransform& transform, const Mesh& cylinderMesh, const Mesh& coneMesh)
    {
        // arrow body
        const mat4x4 curMatrix = transform.toMatrix();
        const Matrix M = ATMath::glmToRayMatrix(curMatrix);
        DrawMesh(cylinderMesh, arrowMaterial, M);
        // arrow head
        // THE ONLY time we are allowed to scale in ANIM trainer is when scaling meshes or things to render.
        // so scaling calls must be placed just render...
        const Matrix coneLocal = MatrixScale(0.02f, 0.02f, 0.02f) * MatrixTranslate(0, 1, 0);
        const Matrix coneFinal = MatrixMultiply(coneLocal, M);
        DrawMesh(coneMesh, arrowMaterial, coneFinal);
    }

    static void DrawCircleAroundAxis(const float radius, const vec3& rotationAxis, const Color color)
    {
        const vec3 perpendicular = ATMath::perpendicular(rotationAxis);
        const vec3 cross = glm::cross(rotationAxis, perpendicular);
        const vec3 right = glm::cross(rotationAxis, cross);
        rlBegin(RL_LINES);
        for (int i = 0; i < 360; i += 10)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            const float radStart = glm::radians(static_cast<float>(i));
            const float radEnd = glm::radians(static_cast<float>(i + 10));
            const vec3 av = radius * ( cross * sinf(radStart) + right * cosf(radStart) );
            const vec3 bv = radius * ( cross * sinf(radEnd) + right * cosf(radEnd) );
            rlVertex3f(av.x, av.y, av.z);
            rlVertex3f(bv.x, bv.y, bv.z);
        }
        rlEnd();
    }
};

void UpdateGizmo(float& cAngle, TransformGizmo& tGiz)
{
    const double scaledTime = GetTime() * .5f;
    cAngle += GetFrameTime();
    if (cAngle > std::numbers::pi)
        cAngle = 0;

    const vec3 cPos {
        static_cast<float>(3 * std::cos(scaledTime)),
        0,
        static_cast<float>(3 * std::sin(scaledTime))
    };
    const quat rotar = ATMath::rotation(cAngle, ATMath::kVec3Up);
    tGiz.SetRotation(rotar);
    tGiz.SetPosition(cPos);
}

void randomizeTransformGizmo(TransformGizmo& tGiz)
{
    const vec3 rp = ATRandom::randomUnitCubeVector();
    const vec3 randomAxis = ATRandom::randomOnUnitSphereVector();
    tGiz.SetPosition(rp);
    const quat rotation = ATMath::rotation(ATRandom::randomFloatInRange(0, 360), randomAxis);
    tGiz.SetRotation(rotation);
}

// DrawGuard always comes before CameraRenderGuard.
// DrawGuard sets up canvas for drawing into.
// CameraRenderGuard sets up camera for drawing. You can change cameras.
struct DrawGuard
{
    DrawGuard()
    {
        BeginDrawing();
    }
    ~DrawGuard()
    {
        EndDrawing();
    }
};

int main(int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, TITLE.data());
    DisableCursor();
    SetTargetFPS(120);

    ATCamera::CameraController cameraController;
    Material gizmoMat = LoadMaterialDefault();
    std::unique_ptr<TransformGizmo> transformGizmo = std::make_unique<TransformGizmo>();
    //randomizeTransformGizmo(*transformGizmo);

    while (!WindowShouldClose())
    {
        cameraController.Update();
        DrawGuard drawGuard;
        constexpr Color backgroundColor(84, 82, 77, 255);
        ClearBackground(backgroundColor);
        {
            ATCamera::CameraRenderGuard cameraRenderGuard(cameraController);

            TransformGizmo& tGiz = *transformGizmo;
            //tGiz.Update(cameraController);
            //tGiz.DrawBB();
            //tGiz.DrawTransformGizmo(gizmoMat);
            tGiz.DrawRotationGizmo();

            DrawSphere({0,0,0}, .05, BLACK);
            DrawSphere({1,1,1}, .05, WHITE);
            DrawSphere({1,0,0}, .05, RED);
            DrawSphere({0,1,0}, .05, GREEN);
            DrawSphere({0,0,1}, .05, BLUE);
            DrawGrid(30, 1.0f);
        }
    }

    UnloadMaterial(gizmoMat);
    CloseWindow();
    return 0;
}
