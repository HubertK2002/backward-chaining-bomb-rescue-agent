# Projekt 2 — SDL

Graficzna wersja symulacji agenta z wnioskowaniem wstecz. SDL pokazuje mapę, cel agenta, koszt oraz historię decyzji. Sterowanie pozwala wykonywać kroki ręcznie albo uruchomić symulację automatycznie.

## Zależności

- CMake 3.20 lub nowszy
- Ninja
- kompilator C++17
- SDL2 oraz SDL2_ttf
- czcionka DejaVu Sans, Liberation Sans albo Arial

Ubuntu / Debian:

```sh
sudo apt install build-essential cmake ninja-build pkg-config libsdl2-dev libsdl2-ttf-dev fonts-dejavu-core
cmake --preset ninja-debug
cmake --build --preset ninja-debug
./build/project2
```

Windows z vcpkg (triplet x64-windows):

```powershell
vcpkg install sdl2 sdl2-ttf --triplet x64-windows
cmake --preset ninja-debug -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build --preset ninja-debug
```

Wybór mapy: strzałki i Enter. Podczas symulacji `Space` wykonuje krok, Enter uruchamia/zatrzymuje automatykę, `R` resetuje symulację, `M` wraca do wyboru mapy, strzałki przewijają historię, a Esc zamyka okno.
