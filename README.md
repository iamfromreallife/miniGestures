# miniGestures

miniGestures is a lightweight (total exe size less than 700 KB), high-performance mouse gesture utility for Windows, written entirely in native C++ using Win32 APIs and optimized for fast gesture recognition, low resource consumption and no runtime dependencies.

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

## Screenshots

<img width="864" height="777" alt="Captura de ecrã 2026-11-24 191517" src="https://github.com/user-attachments/assets/64d0677d-a1d6-422a-a010-34b46b3d893b" />
<img width="1094" height="917" alt="Captura de ecrã 2026-11-24 191500" src="https://github.com/user-attachments/assets/310274ea-8092-482d-84bc-8649407aa570" />
<img width="1092" height="906" alt="Captura de ecrã 2026-11-24 191436" src="https://github.com/user-attachments/assets/0dd5dba8-d3d5-40ae-aa62-30b0ccd7931d" />
<img width="1083" height="905" alt="Captura de ecrã 2026-11-24 191351" src="https://github.com/user-attachments/assets/3929f182-3a97-4f67-9b8b-31bb5443b46b" />
<img width="1419" height="1054" alt="Captura de ecrã 2026-11-24 183045" src="https://github.com/user-attachments/assets/cc2240ff-6b97-4e38-9fc4-ff6eabc9ad26" />
<img width="1130" height="989" alt="Captura de ecrã 2026-11-24 183022" src="https://github.com/user-attachments/assets/b0d16b7a-dad8-47b4-8624-fc7d92d63c3a" />
<img width="973" height="848" alt="Captura de ecrã 2026-11-24 183003" src="https://github.com/user-attachments/assets/5597590c-526d-4f02-8a4c-6b0207ee06c3" />



## License

MIT — see [LICENSE](LICENSE).
