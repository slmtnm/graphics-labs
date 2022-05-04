#include "spotlight.h"

SpotLight::SpotLight() :
		position(0.0f, 0.0f, 0.0f, 1.0f),
		direction(0.0f, 0.0f, 0.0f, 1.0f),
		cutoff(0.0f), intensity(0.0f) {}

SpotLight::SpotLight(XMFLOAT3 const& position, XMFLOAT3 const& direction,
		XMFLOAT3 const& color, float cutoffDegree, float intensity) : intensity(intensity) {
	this->position = XMFLOAT4(position.x, position.y, position.z, 1.0f);
	this->direction = XMFLOAT4(direction.x, direction.y, direction.z, 1.0f);
	this->color = XMFLOAT4(color.x, color.y, color.z, 1.0f);
	this->cutoff = cosf(XMConvertToRadians(cutoffDegree));
}

void SpotLight::setIntensity(float intensity) {
	this->intensity = intensity;
}

XMFLOAT4 SpotLight::getPosition() const { return position; }

XMFLOAT4 SpotLight::getDirection() const { return direction; }

XMFLOAT4 SpotLight::getColor() const { return color;  }

float SpotLight::getCutoff() const { return cutoff; }

float SpotLight::getIntensity() const { return intensity; }
