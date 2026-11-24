# miniGestures

miniGestures is a lightweight, high-performance mouse gesture utility for Windows, written entirely in native C++ using Win32 APIs and optimized for fast gesture recognition, low resource consumption and no runtime dependencies.

## Features

1. Fast, local gesture recognition using a custom native matching engine.
2. Global mouse gestures that work across applications.
3. Application-specific gesture profiles with optional inheritance of global gestures.
4. Custom drawn shapes, multiple shapes per gesture and built-in templates.
5. Rocker gestures using mouse-button combinations.
6. Mouse-wheel gestures and configurable activation buttons and modifiers.
7. Keyboard hotkeys, including single-key action shortcuts.
8. Multiple actions per gesture, executed sequentially or simultaneously.
9. Window management, including minimize, maximize, close and center.
10. Virtual desktop navigation, direct first/last desktop switching and Task View.
11. Browser navigation and tab-management actions.
12. Program launching, elevated launching and process termination.
13. Application exclusions and wildcard/regex matching by executable, path, window class and title.
14. Customizable gesture trail, sensitivity and minimum movement distance, with optional taskbar capture.
15. Portable JSON settings with import/export, tray operation, Windows startup, administrator-window support, light/dark themes and six interface languages.

## Prebuilt releases

Simply download from the releases page
https://github.com/iamfromreallife/miniGestures/releases

## Compile from source (Windows x64)

1. Install [Visual Studio or Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/) with the **Desktop development with C++** workload, MSVC x64/x86 tools and a Windows SDK.
2. Clone this repository or download and extract its source ZIP.
3. Double-click `compile.bat` in the repository root (or run it from Command Prompt).
4. Launch the generated `miniGestures.exe` in the repository root.

The script locates the Microsoft C++ toolchain, compiles the C++17 sources and Windows resources, and links a standalone x64 executable using the static C++ runtime. No Go, Python, CMake or separate Visual C++ Redistributable is required. Use `compile.bat --no-pause` for non-interactive builds.

Settings are stored in `miniGestures.json` alongside the executable. Keep that file when upgrading and store the executable and settings in a trusted directory, especially when running with administrator rights.


## License

MIT — see [LICENSE](LICENSE).
