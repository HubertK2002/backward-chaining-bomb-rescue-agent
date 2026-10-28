cmake_minimum_required(VERSION 3.20)
project(project2_reasoning_agent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EMSCRIPTEN)
    find_file(PROJECT2_WEB_FONT DejaVuSans.ttf
        PATHS /usr/share/fonts/truetype/dejavu /usr/share/fonts/truetype/dejavu-sans
        NO_CMAKE_FIND_ROOT_PATH
        REQUIRED)
else()
    find_package(SDL2 CONFIG QUIET)
    find_package(SDL2_ttf CONFIG QUIET)
    if(NOT TARGET SDL2::SDL2 OR NOT TARGET SDL2_ttf::SDL2_ttf)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
            pkg_check_modules(SDL2_TTF QUIET IMPORTED_TARGET SDL2_ttf)
        endif()
        if(TARGET PkgConfig::SDL2 AND TARGET PkgConfig::SDL2_TTF)
            set(SDL2_TARGET PkgConfig::SDL2)
            set(SDL2_TTF_TARGET PkgConfig::SDL2_TTF)
        else()
            message(FATAL_ERROR "SDL2 and SDL2_ttf development packages are required. Install libsdl2-dev and libsdl2-ttf-dev, or provide their CMake packages.")
        endif()
    else()
        set(SDL2_TARGET SDL2::SDL2)
        set(SDL2_TTF_TARGET SDL2_ttf::SDL2_ttf)
    endif()
endif()

add_executable(project2
    main.cpp sdl_ui.cpp agent.cpp environment.cpp knowledge_base.cpp knowledge_reasoner.cpp)
target_compile_definitions(project2 PRIVATE SDL_MAIN_HANDLED)
if(EMSCRIPTEN)
    target_compile_options(project2 PRIVATE -sUSE_SDL=2 -sUSE_SDL_TTF=2)
    target_link_options(project2 PRIVATE
        -sUSE_SDL=2 -sUSE_SDL_TTF=2 -sASYNCIFY=1 -sALLOW_MEMORY_GROWTH=1 -sMINIFY_HTML=0
        "SHELL:--preload-file \"${CMAKE_CURRENT_SOURCE_DIR}/maps@/maps\" --preload-file \"${PROJECT2_WEB_FONT}@/fonts/DejaVuSans.ttf\"")
    set_target_properties(project2 PROPERTIES SUFFIX ".html")
else()
    target_link_libraries(project2 PRIVATE ${SDL2_TARGET} ${SDL2_TTF_TARGET})
endif()
if(MSVC)
    target_compile_options(project2 PRIVATE /W4)
else()
    target_compile_options(project2 PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(NOT EMSCRIPTEN)
    add_custom_command(TARGET project2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/maps $<TARGET_FILE_DIR:project2>/maps)
endif()
