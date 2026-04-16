#include <iostream>
#include <raylib.h>

#include "ATCamera.h"
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
class TransormGizmo
{
    Vector3 position = {0,0,0};
    Quaternion rotation = {0,0,0,1};
    BoundingBox boundingBox = {Vector3{0,0,0}, {1,1,1}};

public:
    [[nodiscard]] Matrix GetCurrentTransform() const
    {
        constexpr Vector3 defaultScale = {1,1,1};
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
        const Matrix currentTMat = GetCurrentTransform();
        rlPushMatrix();
            rlMultMatrixf(&currentTMat.m0);

            rlBegin(RL_LINES);
                DrawBoundingBox(boundingBox, PURPLE);
            rlEnd();
        rlPopMatrix();
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
    std::unique_ptr<TransormGizmo> transformGizmo = std::make_unique<TransormGizmo>();

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        cameraController.Update();
        BeginDrawing();
        {
            ClearBackground(RAYWHITE);
            {
                ATCamera::CameraRenderGuard cameraRenderGuard(cameraController);
                {
                    const double deltaTime = GetTime();
                    const Vector3 cPos {
                        static_cast<float>(3 * std::cos(deltaTime)),
                        static_cast<float>(3 * std::sin(deltaTime)),0
                    };
                    transformGizmo->SetPosition(cPos);

                    transformGizmo->DrawTransformGizmo(gizmoMat);
                    transformGizmo->DrawBB();

                    DrawTransformGizmo(gizmoMat);
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