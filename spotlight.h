#pragma once

#include <directxmath.h>

class SpotLight {
	// spotlight's position
	XMVECTOR position;

	// the direction the spotlight is aiming at
	XMVECTOR direction;

	// angle that specifies the spotlight's radius
	float cutoff;

	SpotLight(XMVECTOR position, XMVECTOR direction, float cutoff) : position(position), direction(direction), cutoff(cutoff) {}
};
