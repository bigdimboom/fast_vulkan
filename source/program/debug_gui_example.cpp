#include "debug_gui_example.h"

#include "../engine/imgui/imgui.h"
#include "../engine/imgui/imgui_impl_vulkan.h"
#include "../engine/imgui/imgui_impl_sdl.h"

#include <lodepng.h>

namespace program
{



DebugGuiExample::DebugGuiExample()
	: app::VulanAppBase()
{
}

DebugGuiExample::DebugGuiExample(int argc, const char** argv)
	: app::VulanAppBase(argc, argv)
{
}

DebugGuiExample::~DebugGuiExample()
{
}

bool DebugGuiExample::initialize()
{
	setInput(getptr());

	auto eye = glm::vec3(0.0f, 0.0f, 4.0f);
	auto width = window().clentrez().x;
	auto height = window().clentrez().y;
	auto fov = 45.0f;
	auto range = glm::vec2(0.01, 5000.0f);

	vkapi::CtxSettings settings;
	d_vkContext = std::make_shared<vkapi::Context>(window(), settings);

	d_camera = std::make_shared<camera::FreeCamera>(eye, width, height, fov, range);

	//d_renderer = std::make_shared<renderer::Renderer>(window(), d_camera);

	d_overlay = std::make_unique<gui::VkOverlay>(evDispatcher(), window(), d_vkContext);

	evDispatcher().listen(std::function<void(const SDL_Event & ev)>([this](const SDL_Event& ev)
	{
		switch (ev.window.event)
		{
		case SDL_WINDOWEVENT_RESTORED:
			d_shouldDraw = true;
			break;
		case SDL_WINDOWEVENT_MINIMIZED:
			d_shouldDraw = false;
			break;
		}
	}));

	std::vector<glm::vec3> vertex(100);
	auto data = d_vkContext->createVertexBufferObject(vertex);

	VmaAllocationCreateInfo image_alloc_info = {};
	image_alloc_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
	auto familyQueueIndex = d_vkContext->familyQueueIndex();
	auto image = d_vkContext->createSharedImageObject(vk::ImageCreateInfo(
		vk::ImageCreateFlags(),
		vk::ImageType::e2D,
		vk::Format::eR8G8B8A8Unorm,
		vk::Extent3D(vk::Extent2D(100, 100), 1),
		1u, 1u,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::SharingMode::eExclusive,
		1,
		&familyQueueIndex,
		vk::ImageLayout::eUndefined),
		image_alloc_info);



	std::vector<unsigned char> buffer;
	std::vector<unsigned char> output;
	lodepng::State state;
	unsigned int w, h;

	if (lodepng::load_file(buffer, "assets/rose.png") == 0)
	{
		if (lodepng::decode(output, w, h, state, buffer) != 0)
		{
			assert(0);
		}
	}

	d_debugDraw = std::make_unique<dd::VkDDRenderInterface>(d_vkContext);

	return true;
}

void DebugGuiExample::update(app::Timepoint now, app::Elapsed elapsed)
{
	d_debugDraw->update(d_camera->viewProj());
}

void DebugGuiExample::render()
{
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	//signal recording imgui command
	d_overlay->begin();

	{
		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
		ImGui::ColorEdit3("clear color", (float*)& clear_color); // Edit 3 floats representing a color
		ImGui::End();
	}

	d_overlay->drawDemo();
	d_overlay->drawFPS();
	d_overlay->end();

	//d_debugDraw->drawDemoExample(*d_camera);
	//dd::xzSquareGrid(-50.0f, 50.0f, -1.0f, 1.7f, dd::colors::Green); // Grid from -50 to +50 in both X & Z
	const ddVec3_In oo = { 0.0,0.0,0.0 };
	const ddVec3_In cc = { 1.0,0.0,1.0 };
	dd::point(oo, cc, 30.0, 0, false);
	d_debugDraw->drawLabel(glm::vec3(0.0f, 0.0f, 0.0f), { 0.8f, 0.0f, 0.3f }, "this is origin", 2.0f, d_camera.get());
	d_debugDraw->drawDemoExample(*d_camera);

	if (d_shouldDraw)
	{
		d_vkContext->setClearValue(clear_color.x, clear_color.y, clear_color.z);
		d_vkContext->frameBegin();

		d_debugDraw->prepare();

		d_vkContext->beginRenderPass();

		d_debugDraw->render();

		d_overlay->render();

		d_vkContext->endRenderPass();
		d_vkContext->frameEnd();

		d_vkContext->framePresent();

		//d_renderer->render();
	}
}

void DebugGuiExample::cleanup()
{
	d_vkContext->vkDevice().waitIdle();
	d_overlay = nullptr;
	d_debugDraw = nullptr;
	d_camera = nullptr;
	//d_renderer = nullptr;
	d_vkContext = nullptr;
}

void DebugGuiExample::handleKeyboard(SDL_Keycode keycode)
{
	static float speed = 0.1f;

	switch (keycode)
	{
	case SDLK_q:
	case SDLK_ESCAPE:
		quitApp();
		break;
	case 'a':
		d_camera->translateRight(-speed);
		break;
	case 'd':
		d_camera->translateRight(speed);
		break;
	case 'w':
		d_camera->translateForward(speed);
		break;
	case 's':
		d_camera->translateForward(-speed);
		break;
	case '[':
		d_camera->translateUp(-speed);
		break;
	case ']':
		d_camera->translateUp(speed);
		break;
	}
}

void DebugGuiExample::handleMouse(float xpos, float ypos, bool& firstTouch)
{
	static float lastX = window().clentrez().x / 2.0f;
	static float lastY = window().clentrez().y / 2.0f;

	if (firstTouch)
	{
		lastX = xpos;
		lastY = ypos;
		firstTouch = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;  // Reversed since y-coordinates go from bottom to left

	lastX = xpos;
	lastY = ypos;

	d_camera->pitch(yoffset * 0.08f);
	d_camera->yaw(xoffset * 0.08f);
}

} // end namespace program
