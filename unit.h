#pragma once

#include <memory>

class Graphics;

class Unit
{
public:
	virtual bool init(std::shared_ptr<Graphics>) = 0;
	virtual void prepareForRender(std::shared_ptr<Graphics>) = 0;
	virtual void render(std::shared_ptr<Graphics>) = 0;
	virtual void cleanup(std::shared_ptr<Graphics>) = 0;
	virtual ~Unit() {};
};
