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
#include <glm/gtx/vector_angle.hpp>

#include "glm/gtx/string_cast.hpp"

#include <ATSceneGraph.h>
#include <Nodes/ATDebugSphere.h>

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

using ATScene::ATSceneGraph;
using NID = ATScene::NodeID;
using ATScene::AttrHandleArray;
using ATScene::ATAttributeHandle;
using ATScene::AttributeDataType;


void ATDrawLine(const vec3& start, const vec3& end, const Color& color)
{
    const Vector3 sv{start.x, start.y, start.z};
    const Vector3 ev{end.x, end.y, end.z};
    DrawLine3D(sv, ev, color);
}

class TransformGizmo
{
    enum class ActiveGizmoTool
    {
        Translation,
        Rotation
    };

    ATTransform _transform;
    // bounding box / extents
    vec3 _hsize = glm::one<vec3>();

    // will delete at some point
    // a Transform gizmo that is "locked" skips Update
    // but can still draw. This is useful for setting a reference point in scene,
    // but do not want to incur the heaviness of the update methods.
    bool _locked = false;
    ActiveGizmoTool _activeGizmoTool = ActiveGizmoTool::Translation;
    ATMath::Axis _selectedAxis = ATMath::Axis::None;
    vec3 _lastAxisPosition = ATMath::kVec3Zero;
    vec3 _lastSphericalPosition = ATMath::kVec3Zero;

    // very inefficient as we create new mesh for every gizmo.
    // but must fix later. Better than mem-leak I had before...
    Material _gizmoMat = LoadMaterialDefault();
    Mesh _coneMesh = GenMeshCone(1.33f, 3, 15);
    Mesh _cylinderMesh = GenMeshCylinder(0.01, 1, 15);

public:
    ~TransformGizmo()
    {
        UnloadMesh(_coneMesh);
        UnloadMesh(_cylinderMesh);
        UnloadMaterial(_gizmoMat);
    }

    void SetLocked(const bool locked)
    {
        _locked = locked;
    }
    [[nodiscard]] vec3 GetPosition() const
    {
        return _transform.getPosition();
    }
    void SetPosition(const vec3& p)
    {
        _transform.setPosition(p);
    }
    void SetRotation(const quat& q)
    {
        _transform.setRotation(q);
    }
    void SetSelected(const ATMath::Axis selAxis)
    {
        this->_selectedAxis = selAxis;
    }
    [[nodiscard]] VecPack GetWorldAxes() const
    {
        return _transform.extractAxes();
    }

    void Update(const ATCamera::CameraController& cameraController)
    {
        if (_locked)
        {
            DrawTransformGizmo();
            return;
        }

        if (IsKeyPressed(KeyboardKey::KEY_Q))
        {
            _activeGizmoTool = _activeGizmoTool == ActiveGizmoTool::Translation ?
                ActiveGizmoTool::Rotation : ActiveGizmoTool::Translation;
        }

        if (_activeGizmoTool == ActiveGizmoTool::Translation)
        {
            TranslationUpdate(cameraController);
            DrawTransformGizmo();
            DrawBB();
        }
        else
        {
            // we still want to draw the Translation gizmo, just not receive translation input;
            RotationUpdate(cameraController);
            DrawTransformGizmo();
            DrawRotationGizmo();
        }
    }

    void DrawTransformGizmo() const
    {
        constexpr Color upAxisColor{83, 189, 116, 255};
        constexpr Color selectedColor = PURPLE;

        // draw up arrow (yellow) //rgb(83, 189, 116)
        _gizmoMat.maps[MATERIAL_MAP_DIFFUSE].color =
            _selectedAxis == ATMath::Axis::Y ? selectedColor : upAxisColor;
        DrawArrowGizmo(_gizmoMat, _transform, _cylinderMesh, _coneMesh);

        // draw x-axis arrow (red)
        _gizmoMat.maps[MATERIAL_MAP_DIFFUSE].color =
            _selectedAxis == ATMath::Axis::X ? selectedColor : RED;
        quat rotation = ATMath::rotation(-90, ATMath::kVec3Forward);
        ATTransform rotationTransform;
        rotationTransform.setRotation(rotation);
        ATTransform redArrowTransform = _transform * rotationTransform;
        DrawArrowGizmo(_gizmoMat, redArrowTransform, _cylinderMesh, _coneMesh);

        // Draw z-axis (blue)
        _gizmoMat.maps[MATERIAL_MAP_DIFFUSE].color =
            _selectedAxis == ATMath::Axis::Z ? selectedColor : BLUE;
        rotation = ATMath::rotation(90, ATMath::kVec3Right);
        rotationTransform.setRotation(rotation);
        ATTransform blueArrowTransform = _transform * rotationTransform;
        DrawArrowGizmo(_gizmoMat, blueArrowTransform, _cylinderMesh, _coneMesh);
    }
    void DrawBB() const
    {
        std::array<vec3,8> cubeCorners = ATMath::getCubeCorners(_hsize);
        // mostly needed to avoid needing to convert too many times below
        std::array<Vector3,8> cubeCornersTransformed = {};
        for (std::size_t i = 0; i < cubeCorners.size(); i++)
        {
            const vec3 transformedCC = _transform.transformPosition(cubeCorners[i]);
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
    void DrawRotationGizmo() const
    {
        constexpr Color kSelectedColor{255,0,255, 255};
        constexpr Color kXAxisColor{255,0,0, 255};
        constexpr Color kYAxisColor{0,255,0, 255};
        constexpr Color kZAxisColor{0,0,255, 255};
        const auto [right, up, forward] = _transform.extractAxes();

        const vec3 cpos = _transform.getPosition();

        Color cColor = _selectedAxis == ATMath::Axis::X ? kSelectedColor : kXAxisColor;
        DrawCircleAroundAxis(1, right, cColor, cpos);
        cColor = _selectedAxis == ATMath::Axis::Y ? kSelectedColor : kYAxisColor;
        DrawCircleAroundAxis(1, up, cColor, cpos);
        cColor = _selectedAxis == ATMath::Axis::Z ? kSelectedColor : kZAxisColor;
        DrawCircleAroundAxis(1, forward, cColor, cpos);

        constexpr Color gizmoSphereColor{0,0,0,33};
        DrawSphere(toRLVec(cpos), 1, gizmoSphereColor);
    }

private:
    void TranslationUpdate(const ATCamera::CameraController& cameraController)
    {
        const ATRay mRay = cameraController.GetWorldMouseRay();
        const RayCollision rayCol = ATMath::getRayCollisionOBB(mRay, _transform, _hsize);
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


        vec3 closestAxisPoint;
        vec3 movementDirection;
        const bool leftMouseDown = IsMouseButtonDown(MouseButton::MOUSE_BUTTON_LEFT);
        if (leftMouseDown and _selectedAxis != ATMath::Axis::None)
        {
            // in this if block, if we hold the mouse down, we never allow switch
            // to different axis because it results in sudden jumps in the gizmo.
            if (_selectedAxis == ATMath::Axis::X)
            {
                closestAxisPoint = cpOnXRay;
                movementDirection = right;
            }
            else if (_selectedAxis == ATMath::Axis::Y)
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
            const vec3 v = closestAxisPoint - _lastAxisPosition;
            // movementDirection = the direction of the selected axis...
            // prevents jumping...
            const vec3 newGizmoPos = curGizmoPos + glm::dot(v, movementDirection) * movementDirection;
            SetPosition(newGizmoPos);
        }
        _lastAxisPosition = closestAxisPoint;
    }

    void RotationUpdate(const ATCamera::CameraController& cameraController)
    {
        const ATRay mRay = cameraController.GetWorldMouseRay();
        const vec3 cpos = _transform.getPosition();
        const auto res = ATMath::raySphereIntersection(mRay, cpos, 1);
        if (!res)
            return;

        const vec2 params = res.value();
        const vec3 sphereIntersectPoint = ATMath::evaluateRay(mRay, params.x);
        const vec3 nlsp = glm::normalize(_lastSphericalPosition - cpos);
        const vec3 nsip = glm::normalize(sphereIntersectPoint - cpos);
        const vec3 cross = glm::normalize(glm::cross(nlsp, nsip));
        const vec3 freeLocalAxis = _transform.inverseTransformDirection(cross);
        // axis locking modifier key
        constexpr KeyboardKey kAxisLockingModifierKey = KEY_LEFT_ALT;
        if (IsKeyDown(kAxisLockingModifierKey))
        {
            if (_selectedAxis == ATMath::Axis::None)
            {
                static const auto dominantAxis = [](const vec3& v)
                {
                    const vec3 abs = glm::abs(v);
                    if (abs.x >= abs.y && abs.x >= abs.z)
                        return ATMath::Axis::X;
                    if (abs.y >= abs.x && abs.y >= abs.z)
                        return ATMath::Axis::Y;
                    return ATMath::Axis::Z;
                };
                // On first frame alt is held, snap to the dominant local axis
                _selectedAxis = dominantAxis(freeLocalAxis);
            }
        }
        else
        {
            // Alt released — clear lock so next alt-press picks a fresh axis
            _selectedAxis = ATMath::Axis::None;
        }

        // uncomment to debug intersection point
        //const Vector3 ip{sphereIntersectPoint.x, sphereIntersectPoint.y, sphereIntersectPoint.z};
        //DrawSphere(ip, 0.01f, ORANGE);
        if (IsMouseButtonPressed(MouseButton::MOUSE_BUTTON_LEFT))
        {
            _lastSphericalPosition = sphereIntersectPoint;
            return;
        }

        if (IsMouseButtonDown(MouseButton::MOUSE_BUTTON_LEFT))
        {
            // Skip this frame if points are too close (mouse hasn't moved enough)
            if (glm::length2(cross) < 1e-6f)
            {
                _lastSphericalPosition = sphereIntersectPoint;
                return;
            }
            const float angleBetween = ATMath::signedAngleBetween(nlsp, nsip, cross);
            if (std::abs(angleBetween) < 0.0001f)
                return;

            vec3 axisToUse;
            switch (_selectedAxis)
            {
                case ATMath::Axis::None:
                    axisToUse = freeLocalAxis;
                    break;
                case ATMath::Axis::X:
                    axisToUse = ATMath::kVec3Right;//_transform.inverseTransformDirection(ATMath::kVec3Right);
                    break;
                case ATMath::Axis::Y:
                    axisToUse = ATMath::kVec3Up;;//_transform.inverseTransformDirection(ATMath::kVec3Up);
                    break;
                case ATMath::Axis::Z:
                    axisToUse = ATMath::kVec3Forward;//_transform.inverseTransformDirection(ATMath::kVec3Forward);
                    break;
                default:
                    throw std::exception("Not expecting a new value here...");
            }
            const float finalAngleBetween = ATMath::signedAngleBetween(nlsp, nsip, axisToUse);
            if (std::abs(finalAngleBetween) < 0.0001f)
                return;

            _transform =  _transform * glm::angleAxis(finalAngleBetween, axisToUse);
            _lastSphericalPosition = sphereIntersectPoint;
        }
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

    static void DrawCircleAroundAxis(const float radius, const vec3& rotationAxis, const Color& color, const vec3 offset = ATMath::kVec3Zero)
    {
        const vec3 perpendicular = ATMath::perpendicular(rotationAxis);
        const vec3 cross = glm::normalize(glm::cross(rotationAxis, perpendicular));
        const vec3 right = glm::normalize(glm::cross(rotationAxis, cross));
        rlBegin(RL_LINES);
        for (int i = 0; i < 360; i += 10)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            const float radStart = glm::radians(static_cast<float>(i));
            const float radEnd = glm::radians(static_cast<float>(i + 10));
            const vec3 av = radius * ( cross * sinf(radStart) + right * cosf(radStart) ) + offset;
            const vec3 bv = radius * ( cross * sinf(radEnd) + right * cosf(radEnd) ) + offset;
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

void drawEnvironment(const std::unique_ptr<TransformGizmo>& originGizmo)
{
    DrawSphere({0,0,0}, .05, BLACK);
    DrawSphere({1,0,0}, .05, RED);
    DrawSphere({0,1,0}, .05, GREEN);
    DrawSphere({0,0,1}, .05, BLUE);
    DrawGrid(30, 1.0f);

    originGizmo->DrawTransformGizmo();
}

int main(int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, TITLE.data());
    DisableCursor();
    SetTargetFPS(120);

    ATCamera::CameraController cameraController;
    Material gizmoMat = LoadMaterialDefault();
    std::unique_ptr<TransformGizmo> transformGizmo = std::make_unique<TransformGizmo>();
    std::unique_ptr<TransformGizmo> originGizmo = std::make_unique<TransformGizmo>();
    originGizmo->SetLocked(true);

    // scene graph init
    ATScene::createATSceneGraph();
    ATNode::DebugSphereNodeFactory::registerNode();
    auto sgRef = ATScene::getSceneGraph();
    assert(sgRef.has_value());
    ATSceneGraph& sceneGraph = sgRef.value();

    // node creation
    const NID debugSphereNodeID = sceneGraph.createNode(ATNode::ATDebugSphereNode::nodeTypeID, "My First Node");
    const AttrHandleArray inputHandles = sceneGraph.getNodeInputHandles(debugSphereNodeID);
    const AttrHandleArray outputHandles = sceneGraph.getNodeOutputHandles(debugSphereNodeID);

    const auto posAttrItr = std::ranges::find_if(inputHandles,
        [](const ATAttributeHandle attrHandle) -> bool
    {
        return attrHandle.getDataType() == AttributeDataType::Vec3;
    });
    ATAttributeHandle posAttr = *posAttrItr;
    ATAttributeHandle outPosAttr = outputHandles[0];

    while (!WindowShouldClose())
    {
        cameraController.Update();
        DrawGuard drawGuard;
        constexpr Color backgroundColor(84, 82, 77, 255);
        ClearBackground(backgroundColor);
        {
            ATCamera::CameraRenderGuard cameraRenderGuard(cameraController);

            transformGizmo->Update(cameraController);

            drawEnvironment(originGizmo);
        }
    }


    ATScene::destroyATSceneGraph();

    UnloadMaterial(gizmoMat);
    CloseWindow();
    return 0;
}
