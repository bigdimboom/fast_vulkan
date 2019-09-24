#include "image_utils.h"
#include <lodepng.h>

#define STB_IMAGE_IMPLEMENTATION
#include  "../util/stb_image.h"

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

bool ImageUtility::load(const std::string& file, std::vector<uint8_t>& buffer, int& width, int& height, int& channel)
{
	int nrComponents = 0;
	unsigned char* data = stbi_load(file.c_str(), &width, &height, &nrComponents, STBI_rgb_alpha);

	if (!data)
	{
		return false;
	}

	channel = 4;
	auto size = (size_t)width * (size_t)height * (size_t)channel;
	buffer.resize(size);
	memcpy(buffer.data(), data, size);

	stbi_image_free(data);
	return true;
}

} // end namespace util