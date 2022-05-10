#pragma once

#include <memory>
#include <vector>
#include <chrono>
#include <d3d11_1.h>
#include <directxmath.h>


using namespace DirectX;

class Camera
{
public:
	Camera(XMVECTOR const& position = {0.0f, 0.0f, -50.0f});

	XMMATRIX view() const;
	XMMATRIX projection() const;

	XMVECTOR getRight() const;
	XMVECTOR getDirection() const;

	XMVECTOR getPosition() const;

	void setAspectRatio(const float aspectRatio);
	void move(XMVECTOR delta);
	void rotate(float dx, float dy);

	void updateViewMatrix(XMVECTOR const& direction, XMVECTOR const& right);

private:
	void updateViewMatrix();
	void updateProjectionMatrix();
	void updateDirection();

	XMVECTOR position;
	XMVECTOR direction;

	const float fov = XM_PIDIV2;
	const float nearZ = 0.01f;
	const float farZ = 10000.f;
	float aspectRatio = 1;

	XMMATRIX viewMatrix;
	XMMATRIX projectionMatrix;

	float yaw = 90.0f;
	float pitch = 0.0f;
};

