#pragma once

#include <directxmath.h>

using namespace DirectX;

class SpotLight {
public:
	SpotLight();

	SpotLight(XMFLOAT3 const& position, XMFLOAT3 const& direction,
		float cutoffDegree, float intensity);

	void setIntensity(float intensity);

	XMFLOAT4 getPosition() const;
	XMFLOAT4 getDirection() const;
	float getCutoff() const;
	float getIntensity() const;

private:
	// spotlight's position
	XMFLOAT4 position;
	// the direction the spotlight is aiming at
	XMFLOAT4 direction;
	// angle that specifies the spotlight's radius
	float cutoff;
	// light intensity
	float intensity;
};
