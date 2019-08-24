#pragma once
#include <SDL2/SDL.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace window
{

class VKWindow
{
public:
	VKWindow(int width, int height, const std::string& name, bool fullscreen = false);
	~VKWindow();

	VKWindow(const VKWindow&) = delete;
	VKWindow(VKWindow&&) = delete;
	void operator=(const VKWindow&) = delete;
	void operator=(VKWindow&&) = delete;

	glm::ivec2 pixelrez() const;
	glm::ivec2 clentrez() const;

	const std::string& name() const;
	vk::SurfaceKHR createSurface(vk::Instance vkInstance);
	const std::vector<const char*>& vkInstanceExtensions() const;

	SDL_Window* sdlWindowptr() const;

private:
	std::string d_name;
	SDL_Window* d_window = nullptr;
	std::vector<const char*> d_vkInstanceExtension;
};

}// end namespace window