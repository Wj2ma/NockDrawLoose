#include "NockDrawLoose.hpp"

#include <iostream>
using namespace std;

int main( int argc, char **argv )
{
	std::string luaSceneFile("Assets/world.lua");
	std::string title("Nock, Draw, Loose!");

	CS488Window::launch(argc, argv, new NockDrawLoose(luaSceneFile), 1024, 768, title);

	return 0;
}
