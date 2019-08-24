#include "vulkan_app.h"
#include "system_mgr.h"
#include <filesystem>

namespace app
{

int VulanAppBase::exec()
{
	SystemMgr::instance().start();

	d_window = std::make_unique<window::VKWindow>(
		SystemMgr::instance().settings().window_width,
		SystemMgr::instance().settings().window_height,
		SystemMgr::instance().settings().window_name,
		SystemMgr::instance().settings().is_fullscreen_enabled
		);

	SDL_Event ev;
	evDispatcher().add_event<SDL_Event>();

	if (!d_window || !initialize())
	{
		SDL_Log("system initialization error.");
		d_window = nullptr;
		SystemMgr::instance().shutdown();
		return EXIT_FAILURE;
	}

	if (d_input)
	{
		d_input->setEventBus(evDispatcher());
	}

	auto first = std::chrono::system_clock::now();
	while (true)
	{
		while (SDL_PollEvent(&ev))
		{
			if (ev.type == SDL_QUIT)
			{
				SDL_Log("Program quit after %i ticks", ev.quit.timestamp);
				goto QUIT;
			}

			evDispatcher().trigger(ev);
		}

		auto next = std::chrono::system_clock::now();
		update(next, next - first);
		render();

		first = next;
	}

QUIT:
	cleanup();
	d_window = nullptr;
	SystemMgr::instance().shutdown();
	return EXIT_SUCCESS;
}

void VulanAppBase::setInput(std::shared_ptr<IUserInput> input)
{
	d_input = input;
}

const std::string& VulanAppBase::name() const
{
	return d_name;
}

window::VKWindow& VulanAppBase::window()
{
	return *d_window;
}

se::dispatcher& VulanAppBase::evDispatcher()
{
	return d_ev_disp;
}

const std::vector<std::string>& VulanAppBase::args() const
{
	return d_args;
}

void VulanAppBase::quitApp()
{
	static SDL_Event stop;
	stop.type = SDL_QUIT;
	SDL_PushEvent(&stop);
}

VulanAppBase::VulanAppBase()
{
}

VulanAppBase::VulanAppBase(int argc, const char** argv)
{
	if (argc > 1)
	{
		for (int i = 1; i < argc; ++i)
		{
			d_args.push_back(argv[i]);
		}
	}

	namespace fs = std::experimental::filesystem;
	fs::path p(argv[0]);
	fs::path pp(p.filename());
	auto name = pp.string();

	d_name.clear();
	for (const auto& c : name)
	{
		if (c == '.')
		{
			break;
		}
		d_name.push_back(c);
	}
}

VulanAppBase::~VulanAppBase()
{
}

} //end namespace app