#pragma once

#include <directxmath.h>

using namespace DirectX;

class SpotLight {
public:
	SpotLight();

	SpotLight(XMFLOAT3 const& position, XMFLOAT3 const& direction,
		XMFLOAT3 const& color, float cutoffDegree, float intensity);

	void setIntensity(float intensity);

	XMFLOAT4 getPosition() const;
	XMFLOAT4 getDirection() const;
	XMFLOAT4 getColor() const;
	float getCutoff() const;
	float getIntensity() const;

private:
	// spotlight's color
	XMFLOAT4 color;
	// spotlight's position
	XMFLOAT4 position;
	// the direction the spotlight is aiming at
	XMFLOAT4 direction;
	// angle that specifies the spotlight's radius
	float cutoff;
	// light intensity
	float intensity;
};
