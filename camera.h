#pragma once

#include <memory>
#include <vector>
#include <chrono>
#include <d3d11_1.h>
#include <directxmath.h>


using namespace DirectX;

class Camera {
	XMVECTOR position;
	XMVECTOR direction;

	const float fov = XM_PIDIV2;
	const float nearZ = 0.01f;
	const float farZ = 10000.f;
	float aspectRatio = 1;

	XMMATRIX viewMatrix;
	XMMATRIX projectionMatrix;

	void updateViewMatrix();
	void updateProjectionMatrix();
	void updateDirection();

	float yaw = 90.0f;
	float pitch = 0.0f;
public:
	Camera();

	XMMATRIX view() const;
	XMMATRIX projection() const;

	XMVECTOR getRight() const;
	XMVECTOR getDirection() const;

	XMVECTOR getPosition() const;

	void setAspectRatio(const float aspectRatio);
	void move(XMVECTOR delta);
	void rotate(float dx, float dy);
};

