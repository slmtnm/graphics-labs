#pragma once

#include <d3d11_1.h>
#include <string>
#include "unit.h"

class Primitive;
class Shader;

class UnitSkybox : public Unit
{
public:
	bool init(std::shared_ptr<Graphics>) override;
	void prepareForRender(std::shared_ptr<Graphics>) override;
	void render(std::shared_ptr<Graphics>) override;
	void cleanup(std::shared_ptr<Graphics>) override;
	~UnitSkybox() {}

private:
	std::shared_ptr<Primitive> skyboxPrim;
	ID3D11ShaderResourceView* skyboxSRV = nullptr;
	std::shared_ptr<Shader> skyboxShader;
};
