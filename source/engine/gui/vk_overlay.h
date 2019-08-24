#pragma once

#include <memory>
#include <SDL2/SDL.h>

#include "../vkapi/vk_ctx.h"
#include "../window/vk_window.h"
#include "../event/dispatcher.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_vulkan.h"
#include "../imgui/imgui_impl_sdl.h"

#include "../renderer/irenderer.h"

namespace gui
{

class VkOverlay final : public renderer::IRenderer
{
public:
	VkOverlay(se::dispatcher& dispatcher, window::VKWindow& window, std::shared_ptr<vkapi::Context> vkCtx);
	~VkOverlay();

	void begin();
	void end();
	void render() override;

	void drawFPS();
	void drawDemo();

	ImGuiIO& imgui_io();

private:

	bool d_isRecording = false;

	se::dispatcher& d_dispatcher;
	window::VKWindow& d_window;
	std::shared_ptr<vkapi::Context> d_vkCtx;
	vk::PipelineCache d_pipelineCache = nullptr;
};



} // end namespace gui