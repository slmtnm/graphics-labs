#include "camera.h"

Camera::Camera() : position({ 0.f, 0.f, -5.f, 0.f }), direction({ 0.f, 0.f, 1.f, 0.f }) {
    updateViewMatrix();
    updateProjectionMatrix();
}

void Camera::updateViewMatrix() {
    updateDirection();
    viewMatrix = XMMatrixLookAtLH(position, XMVectorAdd(position, direction), { 0.f, 1.f, 0.f, 0.f });
}

void Camera::updateProjectionMatrix() {
    projectionMatrix = XMMatrixPerspectiveFovLH(fov, aspectRatio, nearZ, farZ);
}

void Camera::updateDirection() {
    float yawRad = XMConvertToRadians(yaw);
    float pitchRad = XMConvertToRadians(pitch);

    direction = XMVectorSet(
        cosf(yawRad) * cosf(pitchRad),
        sinf(pitchRad),
        sinf(yawRad) * cosf(pitchRad),
        0
    );
}

XMMATRIX Camera::view() {
    return viewMatrix;
}

XMMATRIX Camera::projection() {
    return projectionMatrix;
}

void Camera::setAspectRatio(const float aspectRatio) { 
    this->aspectRatio = aspectRatio; 
    updateProjectionMatrix();
}

void Camera::move(XMVECTOR delta) {
    position += delta;
    updateViewMatrix();
}

void Camera::rotate(float dx, float dy) {
    yaw += dx;
    pitch += dy;
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
    updateViewMatrix();
}

XMVECTOR Camera::getRight() {
    return XMVector3Normalize(XMVector3Cross({ 0.0f, 1.0f, 0.0f, 0.0f }, direction));
}

XMVECTOR Camera::getDirection() {
    return direction;
}
