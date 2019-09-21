#include "program/triangle_example.h"
#include "program/debug_gui_example.h"

int main(int argc, const char** argv)
{
	//return std::make_shared<program::TriangleExample>(argc, argv)->exec();
	return std::make_shared<program::DebugGuiExample>(argc, argv)->exec();
}