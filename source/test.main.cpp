#include "program/triangle_example.h"

int main(int argc, const char** argv)
{
	return std::make_shared<program::TriangleExample>(argc, argv)->exec();
}