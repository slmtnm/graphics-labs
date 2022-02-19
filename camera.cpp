#include "camera.h"

Camera::Camera() : position({ 0.f, 0.f, -5.f, 0.f }), direction({ 0.f, 0.f, 1.f, 0.f }) {
    updateViewMatrix();
    updateProjectionMatrix();
}

void Camera::updateViewMatrix() {
    viewMatrix = XMMatrixLookAtLH(position, XMVectorAdd(position, direction), { 0.f, 1.f, 0.f, 0.f });
}

void Camera::updateProjectionMatrix() {
    projectionMatrix = XMMatrixPerspectiveFovLH(fov, aspectRatio, nearZ, farZ);
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

void Camera::rotate(float pitch, float yaw) {
}

XMVECTOR Camera::getRight() {
    return XMVector3Cross({ 0.0f, 1.0f, 0.0f, 0.0f }, direction);
}

XMVECTOR Camera::getDirection() {
    return direction;
}
