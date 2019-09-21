#include "../engine/app/vulkan_app.h"
#include "../engine/app/iuser_input.h"
#include "../engine/renderer/renderer.h"
#include "../engine/camera/free_camera.h"

#include <memory>

namespace program
{

class TriangleExample : public app::VulanAppBase, public app::IUserInput
{
public:
	TriangleExample();
	TriangleExample(int argc, const char** argv);
	~TriangleExample();

	TriangleExample(const TriangleExample&) = delete;
	TriangleExample(TriangleExample&&) = delete;
	void operator=(const TriangleExample&) = delete;
	void operator=(TriangleExample&&) = delete;


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
	std::shared_ptr<renderer::Renderer> d_renderer;
	std::shared_ptr<camera::FreeCamera> d_camera;
};

}