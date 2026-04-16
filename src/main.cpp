#include <iostream>
#include <numbers>
#include <raylib.h>

#include "ATCamera.h"
#include "ATMath.h"
#include "ATRandom.h"

#include "raymath.h"
#include "rlgl.h"


constexpr static std::string_view TITLE = "AnimTrainer";


void DrawArrowGizmo(const Material& arrowMaterial, const Vector3& position, const Matrix& rotation)
{
    const static Mesh coneMesh = GenMeshCone(1.33f, 3, 15);
    const static Mesh cylinderMesh = GenMeshCylinder(0.03, 1, 15);

    const Matrix T = MatrixTranslate(position.x, position.y, position.z);
    const Matrix M = MatrixMultiply(rotation, T);

    // arrow body
    DrawMesh(cylinderMesh, arrowMaterial, M);

    const Matrix coneLocal = MatrixScale(0.1f, 0.1f, 0.1f) * MatrixTranslate(0, 1, 0);
    const Matrix coneFinal = MatrixMultiply(coneLocal, M);
    // arrow head
    DrawMesh(coneMesh, arrowMaterial, coneFinal);
}


// currently, I only deal with position and rotation;
// Will add scale last...
class TransformGizmo
{
    Vector3 position = {0,0,0};
    Quaternion rotation = {0, 0, 0, 1};
    // bounding box / extents
    Vector3 hsize = {1, 1, 1};

    // will delete at some point
    ATMath::Axis selectedAxis = ATMath::Axis::None;


public:
    [[nodiscard]] Matrix GetCurrentTransform() const
    {
        constexpr Vector3 defaultScale = {1,1,1};
        //return MatrixTranslate(position.x, position.y, position.z) * QuaternionToMatrix(rotation);
        return MatrixCompose(position, rotation, defaultScale);
    }
    [[nodiscard]] Vector3 GetPosition() const
    {
        return this->position;
    }
    void SetPosition(const Vector3& p)
    {
        this->position = p;
    }
    [[nodiscard]] Vector3 GetExtents() const
    {
        return this->hsize;
    }
    void SetExtents(const Vector3& extents)
    {
        this->hsize = extents;
    }
    [[nodiscard]] Quaternion GetRotation() const
    {
        return this->rotation;
    }
    void SetRotation(const Quaternion& q)
    {
        this->rotation = q;
    }
    void SetSelected(const ATMath::Axis selAxis)
    {
        this->selectedAxis = selAxis;
    }

    [[nodiscard]] std::tuple<Vector3,Vector3,Vector3> GetWorldAxes() const
    {
        const Matrix rot = QuaternionToMatrix(rotation);
        const Vector3 right{rot.m0, rot.m1, rot.m2};
        const Vector3 up{rot.m4, rot.m5, rot.m6};
        const Vector3 forward{rot.m8, rot.m9, rot.m10};
        return {right, up, forward};
    }

    void DrawTransformGizmo(const Material& transformMat) const
    {
        constexpr Vector3 origin{0,0,0};
        constexpr Color upAxisColor{83, 189, 116, 255};
        constexpr Color selectedColor = PURPLE;

        // draw up arrow (yellow) //rgb(83, 189, 116)
        transformMat.maps[MATERIAL_MAP_DIFFUSE].color =
            selectedAxis == ATMath::Axis::Y ? selectedColor : upAxisColor;
        const Matrix currentTMat = GetCurrentTransform();
        DrawArrowGizmo(transformMat, origin, currentTMat);

        // draw x-axis arrow (red)
        transformMat.maps[MATERIAL_MAP_DIFFUSE].color =
            selectedAxis == ATMath::Axis::X ? selectedColor : RED;
        Vector3 axis{0,0,1};
        const Matrix redArrowMat = MatrixRotate(axis, -90 * DEG2RAD) * currentTMat;
        DrawArrowGizmo(transformMat, origin, redArrowMat);

        // Draw z-axis (blue)
        transformMat.maps[MATERIAL_MAP_DIFFUSE].color = BLUE;
        axis.z = 0;
        axis.x = 1;
        const Matrix blueArrowMat = MatrixRotate(axis, 90 * DEG2RAD) * currentTMat;
        DrawArrowGizmo(transformMat, origin, blueArrowMat);
    }
    void DrawBB() const
    {
        std::array<Vector3, 8> cubeCorners = ATMath::getCubeCorners(hsize);
        for (auto& corner : cubeCorners)
            corner = Vector3Transform(corner, GetCurrentTransform());

        // Draw the 12 edges using DrawLine3D
        // Back face
        DrawLine3D(cubeCorners[0], cubeCorners[1], ORANGE);
        DrawLine3D(cubeCorners[1], cubeCorners[2], ORANGE);
        DrawLine3D(cubeCorners[2], cubeCorners[3], ORANGE);
        DrawLine3D(cubeCorners[3], cubeCorners[0], ORANGE);
        // Front face
        DrawLine3D(cubeCorners[4], cubeCorners[5], ORANGE);
        DrawLine3D(cubeCorners[5], cubeCorners[6], ORANGE);
        DrawLine3D(cubeCorners[6], cubeCorners[7], ORANGE);
        DrawLine3D(cubeCorners[7], cubeCorners[4], ORANGE);

        // Connecting lines
        for (int i = 0; i < 4; i++)
            DrawLine3D(cubeCorners[i], cubeCorners[i + 4], ORANGE);
    }

    static void DrawArrowGizmo(const Material& arrowMaterial, const Vector3& position, const Matrix& rotation)
    {
        const static Mesh coneMesh = GenMeshCone(1.33f, 3, 15);
        const static Mesh cylinderMesh = GenMeshCylinder(0.03, 1, 15);

        const Matrix T = MatrixTranslate(position.x, position.y, position.z);
        const Matrix M = MatrixMultiply(rotation, T);

        // arrow body
        DrawMesh(cylinderMesh, arrowMaterial, M);

        const Matrix coneLocal = MatrixScale(0.1f, 0.1f, 0.1f) * MatrixTranslate(0, 1, 0);
        const Matrix coneFinal = MatrixMultiply(coneLocal, M);
        // arrow head
        DrawMesh(coneMesh, arrowMaterial, coneFinal);
    }
};

RayCollision GetRayCollisionOBB(const Ray& worldRay, const Matrix& transform, const Vector3& size)
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

void RayCollisionLogic(const ATCamera::CameraController& cameraController, TransformGizmo& transformGizmo)
{
    transformGizmo.SetSelected(ATMath::Axis::None);
    const Matrix ct = transformGizmo.GetCurrentTransform();
    const Vector3 extents = transformGizmo.GetExtents();
    const Ray mRay = cameraController.GetWorldMouseRay();
    const RayCollision rayCol = GetRayCollisionOBB(mRay, ct, extents);
    if (rayCol.hit)
    {
        //x-y plane
        const auto [right, up, forward] = transformGizmo.GetWorldAxes();
        const Vector3 planePos = transformGizmo.GetPosition();
        float rayParam = ATMath::getPlaneRayIntersection(mRay, planePos, right);
        // dot(right, rPlaneIntersec) = 0
        const Vector3 rPlaneIntersec = ATMath::evaluateRay(mRay, rayParam);

        rayParam = ATMath::getPlaneRayIntersection(mRay, planePos, up);
        // dot(up, uPlaneIntersec) = 0
        const Vector3 uPlaneIntersec = ATMath::evaluateRay(mRay, rayParam);

        rayParam = ATMath::getPlaneRayIntersection(mRay, planePos, forward);
        // dot(forward, fPlaneIntersec) = 0
        const Vector3 fPlaneIntersec = ATMath::evaluateRay(mRay, rayParam);

        // now we find the closest arrow
        Vector3 upScaler = up * Vector3DotProduct(fPlaneIntersec - planePos, up);
        Vector3 rightScaler = right * Vector3DotProduct(fPlaneIntersec - planePos, right);

        float upDot = Vector3DotProduct(upScaler, upScaler);
        float rightDot = Vector3DotProduct(rightScaler, rightScaler);
        if (upDot > rightDot)
        {
            transformGizmo.SetSelected(ATMath::Axis::Y);
            upScaler = up * Vector3DotProduct(rPlaneIntersec - planePos, up);
            upDot = Vector3DotProduct(upScaler, upScaler);
            Vector3 forwardScaler = forward * Vector3DotProduct(rPlaneIntersec - planePos, forward);
            const float fDot = Vector3DotProduct(forwardScaler, forwardScaler);
            if (upDot > fDot)
            {
                transformGizmo.SetSelected(ATMath::Axis::Z);
            }
        }
        else
        {
            transformGizmo.SetSelected(ATMath::Axis::X);

            rightScaler = right * Vector3DotProduct(uPlaneIntersec - planePos, right);
            rightDot = Vector3DotProduct(rightScaler, rightScaler);
            Vector3 forwardScaler = forward * Vector3DotProduct(uPlaneIntersec - planePos, forward);
            const float fDot = Vector3DotProduct(forwardScaler, forwardScaler);
            if (rightDot > fDot)
                transformGizmo.SetSelected(ATMath::Axis::Z);
        }

        DrawSphere(uPlaneIntersec, 0.02f, YELLOW);
        DrawLine3D(planePos, uPlaneIntersec, GREEN);
        //DrawLine3D(fPlaneIntersec, upScaler, PURPLE);
        //DrawLine3D(fPlaneIntersec, rightScaler, PURPLE);
    }
}

void UpdateGizmo(float& cAngle, TransformGizmo& tGiz)
{
    constexpr Vector3 axis(0,1,0);
    const double scaledTime = GetTime() * .5f;

    cAngle += GetFrameTime();
    if (cAngle > std::numbers::pi)
        cAngle = 0;

    const Vector3 cPos {
        static_cast<float>(3 * std::cos(scaledTime)),
        0,
        static_cast<float>(3 * std::sin(scaledTime))
    };
    const Quaternion cRotar = QuaternionFromAxisAngle(axis, cAngle);
    tGiz.SetRotation(cRotar);
    tGiz.SetPosition(cPos);
}


void RandomPointMotionTest(Vector3& debugSphereCurPos, Vector3& debugSphereEndPos)
{
    Vector3 rayDir(.2,-.333f,-1);
    rayDir = Vector3Normalize(rayDir);
    Ray ray(Vector3(0,1,3), rayDir);

    const Vector3 moveToward = debugSphereEndPos - debugSphereCurPos;
    if (Vector3DotProduct(moveToward, moveToward) < 0.03f)
    {
        debugSphereEndPos = ATMath::UP + ATRandom::randomUnitCubeVector();
    }
    else
    {
        const float dt = GetFrameTime();
        debugSphereCurPos += moveToward * dt;
    }

    // perform closest point
    const Vector3 toRayPos = ray.position - debugSphereCurPos;
    const float parallel = Vector3DotProduct(toRayPos, ray.direction);
    const Vector3 perpPoint = debugSphereCurPos + toRayPos - ray.direction * parallel;

    DrawLine3D(ray.position, ATMath::evaluateRay(ray, 5), GREEN);
    DrawSphere(debugSphereCurPos, 0.02f, GREEN);

    DrawLine3D(debugSphereCurPos, perpPoint, GREEN);
    DrawSphere(perpPoint, 0.02f, PURPLE);
}



int main(int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, TITLE.data());
    DisableCursor();

    ATCamera::CameraController cameraController;
    Material gizmoMat = LoadMaterialDefault();
    std::unique_ptr<TransformGizmo> transformGizmo = std::make_unique<TransformGizmo>();

    SetTargetFPS(60);

    constexpr Color backgroundColor(84, 82, 77, 255);
    float cAngle = 0.0f;
    Vector3 curSpherePos(0,0,0);
    Vector3 curSphereEndPos(1,1,1);
    while (!WindowShouldClose())
    {
        cameraController.Update();
        BeginDrawing();
        {
            ClearBackground(backgroundColor);
            {
                ATCamera::CameraRenderGuard cameraRenderGuard(cameraController);
                {
                    TransformGizmo& tGiz = *transformGizmo;
                    //UpdateGizmo(cAngle, tGiz);

                    //tGiz.DrawBB();
                    tGiz.DrawTransformGizmo(gizmoMat);

                    // lets shoot a ray
                    //RayCollisionLogic(cameraController, tGiz);

                    RandomPointMotionTest(curSpherePos, curSphereEndPos);

                    DrawGrid(30, 1.0f);
                }
            }
        }
        EndDrawing();
    }

    UnloadMaterial(gizmoMat);
    CloseWindow();
    return 0;
}