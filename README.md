# Violet - LV2 Plugin Host

A lightweight LV2 plugin host for real-time audio processing on Windows.

## Building

### Prerequisites

On Fedora Linux (for cross-compilation):
```bash
sudo dnf install meson ninja-build mingw64-gcc mingw64-gcc-c++
```

### Cross-compilation for Windows

1. Set up the build directory:
```bash
meson setup build --cross-file cross-mingw64.txt
```

2. Compile:
```bash
meson compile -C build
```

3. The executable will be in `build/violet.exe`

### Development Build (with debug console)

```bash
meson setup build-debug --cross-file cross-mingw64.txt -Ddebug=true
meson compile -C build-debug
```

## Running

The application requires Windows 10 or later. Copy the executable to a Windows machine and run it.

## Current Status

**✅ Phase 1 Complete**: Basic Infrastructure
- [x] Project structure and build system
- [x] Basic Windows application with message loop  
- [x] Main window with menus and basic layout
- [x] Configuration system for settings persistence
- [x] Cross-compilation setup for MinGW-w64
- [x] **Successfully building and creating Windows executables**

**🚧 Next Steps**: Audio Engine Implementation
- [ ] WASAPI audio backend
- [ ] LV2 plugin loading (LILV integration)  
- [ ] Basic audio processing pipeline

## Architecture

```
src/
├── main.cpp              # Application entry point
├── audio/               # Audio engine and plugin management
├── ui/                  # User interface components
│   └── main_window.cpp  # Main application window
├── core/                # Core utilities and configuration
│   ├── config_manager.cpp # Settings management
│   └── utils.cpp        # Utility functions
└── platform/            # Platform-specific code
    └── windows_api.cpp  # Windows API wrappers
```

## License

MIT License - see LICENSE file for details.# violet
