#pragma once
#include "../engine/app/vulkan_app.h"
#include "../engine/app/iuser_input.h"
#include "../engine/camera/free_camera.h"
#include "../engine/vkapi/vk_ctx.h"
#include "../engine/gui/vk_overlay.h"
#include "../engine/debug_draw/vk_dd.h"
#include "../engine/mesh/static_model.h"

#include <memory>

namespace program
{

class StaticMeshExample : public app::VulanAppBase, public app::IUserInput
{
public:
	StaticMeshExample();
	StaticMeshExample(int argc, const char** argv);
	~StaticMeshExample();

	StaticMeshExample(const StaticMeshExample&) = delete;
	StaticMeshExample(StaticMeshExample&&) = delete;
	void operator=(const StaticMeshExample&) = delete;
	void operator=(StaticMeshExample&&) = delete;


	// Inherited via VulanAppBase
	virtual bool initialize() override;

	virtual void update(app::Timepoint now, app::Elapsed elapsed) override;

	virtual void render() override;

	virtual void cleanup() override;


	// Inherited via IUserInput
	virtual void handleKeyboard(SDL_Keycode keycode) override;

	virtual void handleMouse(float xpos, float ypos, bool& firstTouch) override;

private:
	bool d_shouldDraw = true;
	std::shared_ptr<camera::FreeCamera> d_camera;
	std::shared_ptr<vkapi::Context> d_vkContext;
	std::unique_ptr<gui::VkOverlay> d_overlay;
	std::unique_ptr<dd::VkDDRenderInterface> d_debugDraw;
	std::shared_ptr<mesh::StaticModel> d_staticModel;
};

}