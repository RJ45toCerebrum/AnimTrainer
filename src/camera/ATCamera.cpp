// Created by Tyler on 4/15/2026.

#include "ATCamera.h"
#include "ATMath.h"

using ATCamera::ATCameraPtr;
using CameraCon = ATCamera::CameraController;
using CamGuard = ATCamera::CameraRenderGuard;


ATCameraPtr ATCamera::buildDefaultCamera()
{
    ATCameraPtr defCamPtr = std::make_unique<Camera3D>();
    setDefaultCameraData(*defCamPtr);
    return defCamPtr;
}

void ATCamera::setDefaultCameraData(Camera3D& camera)
{
    camera.position = Vector3{1, 1, 1};
    camera.target = Vector3{0.0f, 0.0f, 0.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 55.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

CameraCon::CameraController() :
    _camera(buildDefaultCamera())
{}

Camera3D& CameraCon::GetCamera() const
{
    return *_camera;
}

void CameraCon::Update()
{
    // probably should not do this here, but it works for now...
    if (IsKeyPressed(KeyboardKey::KEY_F))
    {
        if (IsCursorHidden())
            EnableCursor();
        else
            DisableCursor();
    }
    UpdateCamera(_camera.get(), CAMERA_FREE);
}

CamGuard::CameraRenderGuard(CameraController& cameraCon) :
    _cameraController(cameraCon)
{
    const Camera3D& cam = cameraCon.GetCamera();
    BeginMode3D(cam);
}
CamGuard::~CameraRenderGuard()
{
    EndMode3D();
}

ATMath::Ray CameraCon::GetWorldMouseRay() const
{
    Camera3D& cam3D = GetCamera();
    const auto [position, dir] = GetScreenToWorldRay(GetMousePosition(), cam3D);
    const glm::vec3 origin{position.x, position.y, position.z};
    const glm::vec3 direction{dir.x, dir.y, dir.z};
    return ATMath::Ray{origin, direction};
}

