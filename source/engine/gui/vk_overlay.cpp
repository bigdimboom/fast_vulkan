#include "vk_overlay.h"
#include <assert.h>

namespace gui
{
VkOverlay::VkOverlay(se::dispatcher& dispatcher, window::VKWindow& window, std::shared_ptr<vkapi::Context> vkCtx)
	: d_dispatcher(dispatcher)
	, d_window(window)
	, d_vkCtx(vkCtx)
{
	assert(d_vkCtx);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	//ImGui::StyleColorsClassic();
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	d_pipelineCache = d_vkCtx->vkDevice().createPipelineCache(vk::PipelineCacheCreateInfo());

	ImGui_ImplSDL2_InitForVulkan(window.sdlWindowptr());
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = d_vkCtx->vkInstance();
	init_info.PhysicalDevice = d_vkCtx->vkPhysicalDevice();
	init_info.Device = d_vkCtx->vkDevice();
	init_info.QueueFamily = d_vkCtx->familyQueueIndex();
	init_info.Queue = d_vkCtx->vkQueue();
	init_info.PipelineCache = d_pipelineCache;
	init_info.DescriptorPool = d_vkCtx->vkDescriptorPool();
	init_info.Allocator = nullptr;
	init_info.MinImageCount = d_vkCtx->nSwapchainFrameBuffers();
	init_info.ImageCount = d_vkCtx->nSwapchainFrameBuffers();
	init_info.CheckVkResultFn = vkapi::Context::check_error;

	ImGui_ImplVulkan_Init(&init_info, d_vkCtx->defaultRenderPass());

	// Upload Fonts
	{
		// Use any command queue
		auto command_buffer = d_vkCtx->beginSingleTimeCommands(true);
		ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
		d_vkCtx->flushSingleTimeCommands(command_buffer, true);
		d_vkCtx->vkDevice().waitIdle();
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	d_dispatcher.listen(std::function<void(const SDL_Event & ev)>([this](const SDL_Event& ev)
	{
		ImGui_ImplSDL2_ProcessEvent(&ev);
	}));

}

VkOverlay::~VkOverlay()
{
	d_vkCtx->vkDevice().waitIdle();
	
	if (d_pipelineCache != vk::PipelineCache(nullptr))
	{
		d_vkCtx->vkDevice().destroyPipelineCache(d_pipelineCache);

	}
	
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void VkOverlay::begin()
{
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame(d_window.sdlWindowptr());
	ImGui::NewFrame();
}

void VkOverlay::end()
{
	ImGui::Render();
}

void VkOverlay::render()
{
	// Record Imgui Draw Data and draw funcs into command buffer
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), d_vkCtx->commandBuffer());
}

void VkOverlay::drawFPS()
{
	const ImGuiWindowFlags fpsWindowFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar;

	ImGui::Begin("fps", 0, fpsWindowFlags);
	ImGui::SetWindowPos(ImVec2(0, 0));
	ImGui::Text("GUI: Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
}

void VkOverlay::drawDemo()
{
	ImGui::ShowDemoWindow();
}

ImGuiIO& VkOverlay::imgui_io()
{
	return ImGui::GetIO();
}
} // end namespace gui