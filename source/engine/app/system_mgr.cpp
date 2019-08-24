#include "system_mgr.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iomanip>

namespace app
{

SystemMgr& SystemMgr::instance()
{
	static SystemMgr inst;
	return inst;
}

void SystemMgr::start(const std::string& config_file_path)
{
	if (!d_inited)
	{
		loadDefaultIfNecessary(config_file_path);
		loadSettings(config_file_path);

		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
			exit(EXIT_FAILURE);
		}

		if (SDL_Vulkan_LoadLibrary("vulkan-1.dll") != 0) {
			SDL_Log("Unable to initialize SDL Vulkan: %s", SDL_GetError());
			exit(EXIT_FAILURE);
		}
	}
}

void SystemMgr::shutdown()
{
	if (d_inited)
	{
		SDL_Vulkan_UnloadLibrary();
		SDL_Quit();
		d_inited = false;
	}
}

const SystemSettings& SystemMgr::settings() const
{
	return d_settings;
}

void SystemMgr::loadDefaultIfNecessary(const std::string& config_file_path)
{
	namespace fs = std::experimental::filesystem;

	fs::path p(config_file_path);

	if (!p.has_extension() || p.extension().string() != ".json")
	{
		SDL_Log("system config must be a json file.");
		exit(EXIT_FAILURE);
	}

	if (!fs::exists(config_file_path))
	{
		if (p.has_parent_path() && !fs::exists(p.parent_path()))
		{
			if (!fs::create_directories(p.parent_path()))
			{
				SDL_Log("system config can't create file/dir");
				exit(EXIT_FAILURE);
			}
		}
		nlohmann::json config;

		config["is_debug_enabled"] = d_settings.is_debug_enabled;
		config["is_cull_enabled"] = d_settings.is_cull_enabled;
		config["is_vsync_enabled"] = d_settings.is_vsync_enabled;
		config["is_fullscreen_enabled"] = d_settings.is_fullscreen_enabled;

		config["window_width"] = d_settings.window_width;
		config["window_height"] = d_settings.window_height;
		config["window_name"] = d_settings.window_name;

		config["depth_bits"] = d_settings.depth_bits;
		config["back_buffer_count"] = d_settings.back_buffer_count;

		config["shader_dir"] = d_settings.shader_dir;

		config["look_sensitivity"] = d_settings.look_sensitivity;
		config["move_sensitivity"] = d_settings.move_sensitivity;

		std::ofstream ofs(config_file_path);

		if (ofs.fail())
		{
			SDL_Log("system config can't create file/dir");
			exit(EXIT_FAILURE);
		}

		ofs << std::setw(4) << config << std::endl;
	}
}

void SystemMgr::loadSettings(const std::string& config_file_path)
{
	namespace fs = std::experimental::filesystem;

	if (!fs::exists(config_file_path))
	{
		SDL_Log("system config can't find file/dir");
		exit(EXIT_FAILURE);
	}

	std::ifstream input(config_file_path);

	if (input.fail())
	{
		SDL_Log("system config can't open file/dir");
		exit(EXIT_FAILURE);
	}

	nlohmann::json config;
	input >> config;

	d_settings.is_debug_enabled = config["is_debug_enabled"];
	d_settings.is_cull_enabled = config["is_cull_enabled"];
	d_settings.is_vsync_enabled = config["is_vsync_enabled"];
	d_settings.is_fullscreen_enabled = config["is_fullscreen_enabled"];

	d_settings.window_width = config["window_width"];
	d_settings.window_height = config["window_height"];
	d_settings.window_name = config["window_name"];

	d_settings.depth_bits = config["depth_bits"];
	d_settings.back_buffer_count = config["back_buffer_count"];

	d_settings.shader_dir = config["shader_dir"];

	if (!fs::exists(d_settings.shader_dir))
	{
		SDL_Log("system can't find %s as shader folder", d_settings.shader_dir.c_str());
		exit(EXIT_FAILURE);
	}

	if (d_settings.shader_dir.back() != '/')
	{
		d_settings.shader_dir.push_back('/');
	}

	d_settings.look_sensitivity = config["look_sensitivity"];
	d_settings.move_sensitivity = config["move_sensitivity"];
}

// hide
SystemMgr::~SystemMgr()
{
}
SystemMgr::SystemMgr()
{
}

} // end namespace app