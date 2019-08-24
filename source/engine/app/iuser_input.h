#pragma once
#include "../event/dispatcher.h"
#include <SDL2/SDL.h>
#include <memory>

namespace app
{

class IUserInput : public std::enable_shared_from_this<IUserInput>
{
public:
	virtual ~IUserInput() {}
	virtual void setEventBus(se::dispatcher& d_ev_disp);
	std::shared_ptr<IUserInput> getptr();
protected:
	virtual void handleKeyboard(SDL_Keycode keycode) = 0;
	virtual void handleMouse(float xpos, float ypos, bool& firstTouch) = 0;
};

} // end namespace 