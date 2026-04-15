#include <iostream>
#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

int main(int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, "AnimTrainer - C++26 Classic");

    Camera camera = { 0 };
    camera.position = Vector3{ 10.0f, 10.0f, 10.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        UpdateCamera(&camera, CAMERA_FREE);
        BeginDrawing();
        {
            ClearBackground(RAYWHITE);
            {
                BeginMode3D(camera);
                {
                    DrawCube(Vector3{ 0, 0, 0 }, 2.0f, 2.0f, 2.0f, RED);
                    DrawCubeWires(Vector3{ 0, 0, 0 }, 2.0f, 2.0f, 2.0f, MAROON);
                    DrawGrid(10, 1.0f);
                }
                EndMode3D();
            }
            if (GuiButton(Rectangle{ 10, 10, 100, 30 }, "Reset Camera")) {
                camera.position = Vector3{ 10.0f, 10.0f, 10.0f };
            }
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}