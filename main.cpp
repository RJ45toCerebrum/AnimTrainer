#include <iostream>
#include <numbers>
#include <raylib.h>

#include "ATCamera.h"
#include "ATMath.h"
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

void DrawTransformGizmo(const Material& transformMat)
{
    constexpr Vector3 origin{0,0,0};

    // draw up arrow (yellow)
    transformMat.maps[MATERIAL_MAP_DIFFUSE].color = YELLOW;
    DrawArrowGizmo(transformMat, origin, MatrixIdentity());

    // draw x-axis arrow (red)
    transformMat.maps[MATERIAL_MAP_DIFFUSE].color = RED;
    Vector3 axis{0,0,1};
    Matrix rotation = MatrixRotate(axis, -90 * DEG2RAD);
    DrawArrowGizmo(transformMat, origin, rotation);

    // Draw z-axis (blue)
    transformMat.maps[MATERIAL_MAP_DIFFUSE].color = BLUE;
    axis.z = 0;
    axis.x = 1;
    rotation = MatrixRotate(axis, 90 * DEG2RAD);
    DrawArrowGizmo(transformMat, origin, rotation);
}

// currently, I only deal with position and rotation;
// Will add scale last...
class TransformGizmo
{
    Vector3 position = {0,0,0};
    Quaternion rotation = {0,0,0,1};
    Vector3 hsize = {1.0f/2, 3, 1.0f/2};

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
    [[nodiscard]] Quaternion GetRotation() const
    {
        return this->rotation;
    }
    void SetRotation(const Quaternion& q)
    {
        this->rotation = q;
    }

    void DrawTransformGizmo(const Material& transformMat) const
    {
        constexpr Vector3 origin{0,0,0};

        // draw up arrow (yellow)
        transformMat.maps[MATERIAL_MAP_DIFFUSE].color = YELLOW;
        const Matrix currentTMat = GetCurrentTransform();
        DrawArrowGizmo(transformMat, origin, currentTMat);

        // draw x-axis arrow (red)
        transformMat.maps[MATERIAL_MAP_DIFFUSE].color = RED;
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

int main(int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, TITLE.data());
    DisableCursor();

    ATCamera::CameraController cameraController;
    Material gizmoMat = LoadMaterialDefault();
    std::unique_ptr<TransformGizmo> transformGizmo = std::make_unique<TransformGizmo>();

    SetTargetFPS(60);

    float cAngle = 0.0f;
    while (!WindowShouldClose())
    {
        cameraController.Update();
        BeginDrawing();
        {
            ClearBackground(RAYWHITE);
            {
                ATCamera::CameraRenderGuard cameraRenderGuard(cameraController);
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
                    transformGizmo->SetRotation(cRotar);
                    transformGizmo->SetPosition(cPos);

                    transformGizmo->DrawBB();
                    transformGizmo->DrawTransformGizmo(gizmoMat);

                    //DrawTransformGizmo(gizmoMat);
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