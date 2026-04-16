#include <iostream>
#include <raylib.h>

#include "ATCamera.h"
#include "raymath.h"

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


int main(int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, TITLE.data());

    ATCamera::CameraController cameraController;
    Material gizmoMat = LoadMaterialDefault();

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
                    const Vector3 cubeP{ 3, 0, 0 };
                    //DrawCube(cubeP, 2.0f, 2.0f, 2.0f, RED);
                    //DrawCubeWires(cubeP, 2.0f, 2.0f, 2.0f, MAROON);

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