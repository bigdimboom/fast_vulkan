#include "image_utils.h"
#include <lodepng.h>

namespace util
{

bool ImageUtility::loadPNG(const std::string& file, std::vector<uint8_t>& buffer, int& width, int& height)
{
	std::vector<uint8_t> raw_buf;

	if (lodepng::load_file(raw_buf, file) != 0)
	{
		return false;
	}

	uint32_t w, h;
	lodepng::State state;
	if (lodepng::decode(buffer, w, h, state, raw_buf) != 0)
	{
		return false;
	}

	width = static_cast<int>(w);
	height = static_cast<int>(h);
	return true;
}

} // end namespace util