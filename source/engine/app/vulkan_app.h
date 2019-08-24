#pragma once
#define SDL_MAIN_HANDLED

#include <chrono>
#include <vector>
#include <memory>
#include "../window/vk_window.h"
#include "../event/dispatcher.h"

#include "iuser_input.h"

namespace app
{

using Timepoint = decltype(std::chrono::system_clock::now());
using Elapsed = decltype(std::chrono::system_clock::now() - std::chrono::system_clock::now());

class VulanAppBase
{
public:
	VulanAppBase();
	VulanAppBase(int argc, const char** argv);
	~VulanAppBase();

	VulanAppBase(const VulanAppBase&) = delete;
	VulanAppBase(VulanAppBase&&) = delete;
	void operator=(const VulanAppBase&) = delete;
	void operator=(VulanAppBase&&) = delete;

	int exec();

protected:

	virtual bool initialize() = 0;
	virtual void update(Timepoint now, Elapsed elapsed) = 0;
	virtual void render() = 0;
	virtual void cleanup() = 0;

	const std::string& name() const;
	window::VKWindow& window();
	se::dispatcher& evDispatcher();
	const std::vector<std::string>& args() const;

	void setInput(std::shared_ptr<IUserInput> input);
	void quitApp();


private:
	std::string d_name = "null";
	std::vector<std::string> d_args;
	std::unique_ptr<window::VKWindow> d_window;
	se::dispatcher d_ev_disp;
	std::shared_ptr<IUserInput> d_input;
};

} //end namespace app