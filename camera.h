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
	const float nearZ = 0.001f;
	const float farZ = 100.f;
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

	XMMATRIX view();
	XMMATRIX projection();

	XMVECTOR getRight();
	XMVECTOR getDirection();

	void setAspectRatio(const float aspectRatio);
	void move(XMVECTOR delta);
	void rotate(float dx, float dy);
};

