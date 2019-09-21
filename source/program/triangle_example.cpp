#include "triangle_example.h"
#include <lodepng.h>

namespace program
{



TriangleExample::TriangleExample()
	: app::VulanAppBase()
{
}

TriangleExample::TriangleExample(int argc, const char** argv)
	: app::VulanAppBase(argc, argv)
{
}

TriangleExample::~TriangleExample()
{
}

bool TriangleExample::initialize()
{
	setInput(getptr());

	auto eye = glm::vec3(0.0f, 0.0f, 4.0f);
	auto width = window().clentrez().x;
	auto height = window().clentrez().y;
	auto fov = 45.0f;
	auto range = glm::vec2(0.01, 5000.0f);


	d_camera = std::make_shared<camera::FreeCamera>(eye, width, height, fov, range);
	d_renderer = std::make_shared<renderer::Renderer>(window(), d_camera);

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


	return true;
}

void TriangleExample::update(app::Timepoint now, app::Elapsed elapsed)
{

}

void TriangleExample::render()
{

	if (d_shouldDraw)
	{
		d_renderer->render();
	}
}

void TriangleExample::cleanup()
{
	d_renderer = nullptr;
}

void TriangleExample::handleKeyboard(SDL_Keycode keycode)
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

void TriangleExample::handleMouse(float xpos, float ypos, bool& firstTouch)
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
