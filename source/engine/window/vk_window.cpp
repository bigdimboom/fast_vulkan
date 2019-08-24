#include "vk_window.h"
#include <SDL2/SDL_vulkan.h>

namespace window
{


VKWindow::VKWindow(int width, int height, const std::string& name, bool full)
	: d_name(name)
{
	int flags = SDL_WINDOW_VULKAN;

	if (full)
	{
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	d_window = SDL_CreateWindow(name.c_str(),
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width, height,
		flags
	);

	if (!d_window) {
		SDL_Log("Unable to initialize SDL Window: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	unsigned int count;
	SDL_Vulkan_GetInstanceExtensions(d_window, &count, nullptr);
	d_vkInstanceExtension.resize(count);
	SDL_Vulkan_GetInstanceExtensions(d_window, &count, d_vkInstanceExtension.data());

//#ifdef VK_EXT_debug_report
//	d_vkInstanceExtension.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
//#endif // VK_EXT_debug_report
}

VKWindow::~VKWindow()
{
	SDL_DestroyWindow(d_window);
}

vk::SurfaceKHR VKWindow::createSurface(vk::Instance vkInstance)
{
	vk::SurfaceKHR surface;
	if (!SDL_Vulkan_CreateSurface(d_window, vkInstance, (VkSurfaceKHR*)& surface))
	{
		SDL_Log(SDL_GetError());
		exit(EXIT_FAILURE);
	}
	return surface;
}

const std::vector<const char*>& VKWindow::vkInstanceExtensions() const
{
	return d_vkInstanceExtension;
}

SDL_Window* VKWindow::sdlWindowptr() const
{
	return d_window;
}

glm::ivec2 VKWindow::pixelrez() const
{
	glm::ivec2 res = {};
	SDL_Vulkan_GetDrawableSize(d_window, &res.x, &res.y);
	return res;
}

glm::ivec2 VKWindow::clentrez() const
{
	glm::ivec2 res = {};
	SDL_GetWindowSize(d_window, &res.x, &res.y);
	return res;
}

const std::string& VKWindow::name() const
{
	return d_name;
}

}// end namespace window