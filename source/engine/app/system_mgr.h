#pragma once
#include <string>

namespace app
{

struct SystemSettings
{
	bool is_debug_enabled = true;
	bool is_cull_enabled = true;
	bool is_vsync_enabled = true;
	bool is_fullscreen_enabled = false;

	int  window_width = 512;
	int  window_height = 512;
	std::string window_name = "debug";

	int depth_bits = 16;
	int back_buffer_count = 2; // double buffer ny default

	std::string shader_dir = "./shaders";

	float look_sensitivity = 1.0f;
	float move_sensitivity = 1.0f;
};

class SystemMgr
{
public:
	~SystemMgr();

	SystemMgr(const SystemMgr&) = delete;
	SystemMgr(SystemMgr&&) = delete;
	void operator=(const SystemMgr&) = delete;
	void operator=(SystemMgr&&) = delete;

	static SystemMgr& instance();

	void start(const std::string& config_file_path = "./system_config.json");
	void shutdown();
	const SystemSettings& settings() const;

private:
	SystemMgr();

	// vars
	bool d_inited = false;
	SystemSettings d_settings;

	// HELPERS;
	void loadDefaultIfNecessary(const std::string& config_file_path);
	void loadSettings(const std::string& config_file_path);
};

} // end namespace app