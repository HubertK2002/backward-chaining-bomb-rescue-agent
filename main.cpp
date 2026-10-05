#include "sdl_ui.h"

#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[]) {
	try {
		std::string mapDirectory = "maps";
		if (!std::filesystem::exists(mapDirectory) && argc > 0)
			mapDirectory = (std::filesystem::path(argv[0]).parent_path() / "maps").string();
		return RunSdlApplication(mapDirectory);
	}
	catch (const std::exception& e) {
		std::cerr << "Application error: " << e.what() << '\n';
		return 1;
	}
}
