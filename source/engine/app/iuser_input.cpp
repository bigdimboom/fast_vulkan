#include "iuser_input.h"

namespace app
{

void IUserInput::setEventBus(se::dispatcher& disp)
{
	disp.listen(std::function<void(const SDL_Event & ev)>([this](const SDL_Event& ev) {

		static bool mouse_on = false;
		static bool firstMouse = true;

		switch (ev.type)
		{
		case SDL_KEYDOWN:
			handleKeyboard(ev.key.keysym.sym);
			break;
		default:
			switch (ev.type) {
			case SDL_MOUSEMOTION:
				if (mouse_on) handleMouse((float)ev.motion.x, (float)ev.motion.y, firstMouse);
				break;
			case SDL_MOUSEBUTTONDOWN:
				switch (ev.button.button)
				{
				case SDL_BUTTON_RIGHT:
					mouse_on = true;
					SDL_SetRelativeMouseMode(SDL_TRUE);
					break;
				}
				break;
			case SDL_MOUSEBUTTONUP:
				switch (ev.button.button)
				{
				case SDL_BUTTON_RIGHT:
					SDL_SetRelativeMouseMode(SDL_FALSE);
					firstMouse = true;
					mouse_on = false;
					break;
				}
				break;
			}
		}
	}));
}

std::shared_ptr<IUserInput> IUserInput::getptr()
{
	return shared_from_this();
}

} // end namespace app
