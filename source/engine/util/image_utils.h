#pragma once
#include <string>
#include <vector>

namespace util
{

class ImageUtility
{
public:
	static bool loadPNG(const std::string& file, std::vector<uint8_t>& buffer, int& width, int& height);
	static bool load(const std::string& file, std::vector<uint8_t>& buffer, int& width, int& height, int& channel);

};


} // end namespace util