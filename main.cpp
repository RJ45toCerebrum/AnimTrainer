#include <iostream>
#include <raylib.h>

#include "ATCamera.h"

constexpr static std::string_view TITLE = "AnimTrainer";

int main(int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, TITLE.data());

    ATCamera::CameraController cameraController;

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
                    DrawCube(Vector3{ 0, 0, 0 }, 2.0f, 2.0f, 2.0f, RED);
                    DrawCubeWires(Vector3{ 0, 0, 0 }, 2.0f, 2.0f, 2.0f, MAROON);
                    DrawGrid(30, 1.0f);
                }
            }
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}