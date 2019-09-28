#include "static_mesh_example.h"
#include <glm/gtc/matrix_transform.hpp>


namespace program
{



StaticMeshExample::StaticMeshExample()
	: app::VulanAppBase()
{
}

StaticMeshExample::StaticMeshExample(int argc, const char** argv)
	: app::VulanAppBase(argc, argv)
{
}

StaticMeshExample::~StaticMeshExample()
{
}

bool StaticMeshExample::initialize()
{
	setInput(getptr());

	auto eye = glm::vec3(0.0f, 0.0f, 4.0f);
	auto width = window().clentrez().x;
	auto height = window().clentrez().y;
	auto fov = 45.0f;
	auto range = glm::vec2(0.01, 100.0f);

	vkapi::CtxSettings settings;
	d_vkContext = std::make_shared<vkapi::Context>(window(), settings);

	d_camera = std::make_shared<camera::FreeCamera>(eye, width, height, fov, range);

	d_overlay = std::make_unique<gui::VkOverlay>(evDispatcher(), window(), d_vkContext);

	d_debugDraw = std::make_unique<dd::VkDDRenderInterface>(d_vkContext);

	d_staticModel = std::make_shared<mesh::StaticModel>("assets/mesh/bedroom/iscv2.obj");

	d_renderer = std::make_unique<renderer::StaticModelRenderer>(d_vkContext, d_camera);
	d_renderer->setModel(d_staticModel, glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0)));
	d_renderer->build(true);

	////auto size = sizeof(octree::LOctantNode<uint32_t>);
	//d_octree = std::make_unique<octree::LinearOctree<uint32_t>>(glm::vec3{-1000.0f, -1000.0f, -1000.0f}, 2000.0f, 10);

	//for (auto mesh : d_staticModel->meshes())
	//{
	//	for (auto ind : mesh->indices)
	//	{
	//		const auto& pt = mesh->positions[ind];
	//		octree::LinearOctree<uint32_t>::ErrorCode code;
	//		auto& data = d_octree->push(pt, code);	
	//	}
	//}

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

	return true;
}

void StaticMeshExample::update(app::Timepoint now, app::Elapsed elapsed)
{
	d_debugDraw->update(d_camera->viewProj());
}

void StaticMeshExample::render()
{
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	//signal recording imgui command
	d_overlay->begin();

	{
		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
		ImGui::ColorEdit3("clear color", (float*)& clear_color); // Edit 3 floats representing a color
		ImGui::End();
	}

	//d_overlay->drawDemo();
	d_overlay->drawFPS();
	d_overlay->end();

	dd::xzSquareGrid(-50.0f, 50.0f, -1.0f, 1.7f, dd::colors::Green, 0, false); // Grid from -50 to +50 in both X & Z
	const ddVec3_In oo = { 0.0f,0.0f,0.0f };
	const ddVec3_In cc = { 1.0f,0.0f,1.0f };
	const ddVec3_In cx = { 0.2f,0.2f,1.0f };
	const glm::mat4 transform(1.0);

	dd::point(oo, cc, 10.0, 0, false);
	dd::axisTriad(*((ddMat4x4_In*)& transform), 0.1f, 1.0f, 0, false);
	//dd::box(oo, cx, 3, 3, 3);
	d_debugDraw->drawLabel(*((glm::vec3*) & oo), { 0.8f, 0.0f, 0.3f }, "(0,0,0)", 1.5, d_camera.get());

	//d_octree->traverse([this](const glm::vec3& min, const glm::vec3& max, std::vector<uint32_t>* data) {
	//	static glm::vec3 color(1.0, 0.0, 0.0);
	//	dd::aabb(*((ddVec3_In*)& min), *((ddVec3_In*)& max), *((ddVec3_In*)& color));
	//	return true;
	//});


	if (d_shouldDraw)
	{
		d_vkContext->setClearValue(clear_color.x, clear_color.y, clear_color.z);
		d_vkContext->frameBegin();

		d_debugDraw->prepare();

		d_vkContext->beginDefaultRenderPass();

		d_renderer->render();

		d_debugDraw->render();

		d_overlay->render();

		d_vkContext->endDefaultRenderPass();

		d_vkContext->frameEnd();
		d_vkContext->framePresent();
	}
}

void StaticMeshExample::cleanup()
{
	d_vkContext->vkDevice().waitIdle();
	d_overlay = nullptr;
	d_debugDraw = nullptr;
	d_camera = nullptr;
	d_vkContext = nullptr;
}

void StaticMeshExample::handleKeyboard(SDL_Keycode keycode)
{
	static float speed = 1.0f;

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

void StaticMeshExample::handleMouse(float xpos, float ypos, bool& firstTouch)
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
