// Created by Tyler on 4/15/2026.

#pragma once

#include <memory>
#include "ATMath.h"
#include "raylib.h"

namespace ATCamera
{
    using ATCameraPtr = std::unique_ptr<Camera3D>;

    ATCameraPtr buildDefaultCamera();
    void setDefaultCameraData(Camera3D& camera);

    /// A camera controller contains a camera
    /// Responsible for:
    /// 1) Taking in inputs and updating the camera
    /// 2) Setting up the gl render state / and ending that state
    /// Note: probably not the best architecture, but early on so this seemed to make most sense.
    /// Will likely need to revisit this once a have something more substantial...
    class CameraController
    {
        // a camera controller contains a camera...
        ATCameraPtr _camera = nullptr;

    public:
        explicit CameraController();
        ~CameraController() = default;
        // disable copy constructor
        CameraController(const CameraController&) = delete;
        CameraController& operator=(const CameraController&) = delete;

        [[nodiscard]] Camera3D& GetCamera() const;
        [[nodiscard]] ATMath::Ray GetWorldMouseRay() const;

        void Update();
    };

    class CameraRenderGuard
    {
        CameraController& _cameraController;

    public:
        explicit CameraRenderGuard(CameraController& cameraCon);
        ~CameraRenderGuard();
    };

}
