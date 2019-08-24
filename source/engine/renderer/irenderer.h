#pragma once

namespace renderer
{

class IRenderer
{
public:
	virtual ~IRenderer() {}
	virtual void render() = 0;
};

}// end namespace renderer