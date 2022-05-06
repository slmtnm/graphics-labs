#pragma once

#include "unit.h"

class Primitive;

class UnitSphereScene : public Unit
{
public:
	bool init(std::shared_ptr<Graphics>) override;
	void prepareForRender(std::shared_ptr<Graphics>) override;
	void render(std::shared_ptr<Graphics>) override;
	void cleanup(std::shared_ptr<Graphics>) override;
	~UnitSphereScene() {}

private:
	const float radius = 2.0f;

	std::shared_ptr<Primitive> spherePrim;
};

