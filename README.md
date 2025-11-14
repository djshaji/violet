# Violet - LV2 Plugin Host

A lightweight LV2 plugin host for real-time audio processing on Windows.

## Screenshots
![vio1](https://github.com/user-attachments/assets/4d64c90a-3a7b-4e20-bdeb-fa14aadbc37d)


## Features

### ✅ Implemented (v0.78)

**Audio Engine**
- ✅ WASAPI audio backend with device enumeration
- ✅ Real-time audio processing with configurable buffer sizes
- ✅ Low-latency audio pipeline (< 10ms roundtrip)
- ✅ Multi-channel audio support
- ✅ Audio device selection (input/output)
- ✅ Sample rate configuration (44.1kHz - 192kHz)
- ✅ Buffer size configuration (64 - 2048 samples)
- ✅ Detailed error messages with troubleshooting guidance

**Plugin Management**
- ✅ LV2 plugin discovery and loading (LILV integration)
- ✅ Plugin parameter management and automation
- ✅ Real-time parameter updates (100ms refresh)
- ✅ Plugin state save/restore infrastructure
- ✅ URID mapping for LV2 features

**MIDI Support**
- ✅ Windows MIDI API integration
- ✅ MIDI input handling
- ✅ MIDI parameter mapping for plugin control

**User Interface**
- ✅ Modern Windows application with menu bar and status bar
- ✅ **Plugin Browser Panel**: Tree view with search and category filtering
- ✅ **Active Plugins Panel**: Vertical list with inline parameter controls
   - Round parameter knobs laid out three per row with live value readouts
  - Bypass toggle button per plugin
  - Remove button per plugin
  - Remove All button to clear chain
  - Auto-expand/collapse functionality
  - Vertical scrolling for many plugins
- ✅ **Plugin Parameters Window**: Floating window with detailed controls
- ✅ **Audio Settings Dialog**: Configure audio devices and format
  - Select input and output audio devices
  - Choose sample rate and buffer size
  - Real-time device enumeration
  - Hot-swap audio devices without restart
- ✅ **Drag-and-Drop**: Drag plugins from browser to active panel to load
- ✅ Plugin loading via double-click from browser
- ✅ CPU usage and latency monitoring in status bar

**Configuration**
- ✅ INI-style configuration system
- ✅ Audio device and buffer size settings
- ✅ Window position and layout persistence
- ✅ Theme preference persistence

**Theme System**
- ✅ Light theme with modern color scheme
- ✅ Dark theme for low-light environments
- ✅ System theme (follows Windows 10/11 preference)
- ✅ View menu for easy theme switching
- ✅ Dark mode title bar integration

**Session Management**
- ✅ Save sessions to .violet files
- ✅ Load sessions with full plugin chain restoration
- ✅ Parameter values preserved across sessions
- ✅ Recent sessions tracking
- ✅ File → New/Open/Save/Save As menu integration

**Real-time Audio Processing**
- ✅ WASAPI audio engine with low-latency processing
- ✅ Auto-start on application launch
- ✅ Real-time plugin chain processing
- ✅ Audio → Start/Stop menu controls
- ✅ Live CPU usage and latency monitoring in status bar
- ✅ Configurable sample rates (44.1kHz - 192kHz)
- ✅ Configurable buffer sizes (64 - 2048 samples)

### 🚧 In Progress

- 🚧 Audio file playback and recording
- 🚧 User manual and comprehensive documentation

### 📋 Planned Features

- Audio file playback and recording
- Individual plugin windows with custom UIs
- Plugin preset management
- Advanced routing and mixing
- Testing framework and comprehensive error handling

## Building

### Prerequisites

On Fedora Linux (for cross-compilation):
```bash
sudo dnf install meson ninja-build mingw64-gcc mingw64-gcc-c++ \
                 mingw64-lilv mingw64-lv2
```

### Cross-compilation for Windows

1. Set up the build directory:
```bash
meson setup build --cross-file cross-mingw64.txt
```

2. Compile:
```bash
ninja -C build
```

3. The executables will be in `build/`:
   - `violet.exe` - GUI application (8.9MB)
   - `violet-console.exe` - Console version with debug output

### Alternative: Quick Build Script

```bash
./build.sh
```

### Building the Windows Installer

Requires NSIS (Nullsoft Scriptable Install System):

1. Prepare the distribution directory:
```bash
mkdir -p /home/djshaji/projects/violet-dist
cp build/violet.exe violet-dist/
cp build/violet-console.exe violet-dist/
cp README.md violet-dist/README.txt
cp LICENSE violet-dist/LICENSE.txt
```

2. Build the installer:
```bash
makensis violet-installer.nsi
```

3. The installer will be created as `violet-0.78-setup.exe`

## Installation

### Windows Installer (Recommended)

1. Download `violet-0.78-setup.exe` from the releases page
2. Run the installer (requires administrator privileges)
3. The installer will:
   - Install Violet to `C:\Program Files\Violet`
   - Create Start Menu shortcuts
   - Create a desktop shortcut
   - Set up the LV2_PATH environment variable
   - Register the application in Add/Remove Programs
   - Launch shortcuts from the Violet install directory for predictable working paths
4. Launch Violet from the Start Menu or desktop shortcut

### Manual Installation

1. Copy the executable to a Windows machine
2. Place LV2 plugins in a directory
3. Set `LV2_PATH` environment variable or run from plugin directory
4. Launch `violet.exe`

### Requirements
- Windows 10 or later (64-bit)
- LV2 plugins (the application will scan standard LV2 paths)
- Audio interface (WASAPI-compatible, built into Windows)

### Using the Application

See [docs/USER_GUIDE.md](docs/USER_GUIDE.md) for comprehensive documentation.

#### Quick Start Guide

1. **First Launch**
   - The audio engine starts automatically with your default audio device
   - If no audio device is detected, go to Audio → Audio Settings

2. **Loading Plugins**
   - Browse available LV2 plugins in the left panel
   - Use the search box to filter plugins by name
   - **Double-click** a plugin to add it to your chain
   - **OR drag-and-drop** from browser to the active plugins panel

3. **Controlling Plugins**
   - **Adjust parameters**: Use inline sliders in the active plugins panel
   - **Real-time updates**: Changes apply immediately to audio
   - **Bypass**: Click the bypass button (B) to disable a plugin temporarily
   - **Remove**: Click the X button to remove a plugin from the chain
   - **Reorder**: Drag plugins up/down in the chain (planned feature)

4. **Audio Configuration**
   - Go to **Audio → Audio Settings** to configure:
     - Input/Output audio devices
     - Sample rate (44.1kHz, 48kHz, 96kHz, 192kHz)
     - Buffer size (64-2048 samples, affects latency)
   - Changes apply immediately (engine restarts if needed)

5. **Managing Sessions**
   - **File → New Session**: Clear current chain and start fresh
   - **File → Save Session**: Save plugin chain with all parameters
   - **File → Open Session**: Load a previously saved setup
   - Sessions are saved as `.violet` files

6. **Monitoring Performance**
   - **CPU Usage**: Displayed in the status bar (lower right)
   - **Latency**: Round-trip audio latency shown in status bar
   - High CPU usage? Increase buffer size in Audio Settings

7. **Themes**
   - **View → Theme**: Choose Light, Dark, or System theme
   - Dark theme is ideal for low-light studio environments
   - Theme preference is saved automatically

## Project Status

**Overall Completion**: ~94%

**Completed Phases**:
- ✅ Phase 1: Core Infrastructure (100%)
- ✅ Phase 2: Audio Engine Foundation (100%)
- ✅ Phase 3: Plugin Management (100%)
- ✅ Phase 4: User Interface Implementation (100%)
- 🔄 Phase 5: Advanced Features (70%)

**Current Focus**: Audio file I/O, testing, and documentation

**Current Focus**: Enhancing UI with drag-and-drop and theme system

See [PROJECT_OUTLINE.md](PROJECT_OUTLINE.md) for detailed development roadmap.

## Architecture

```
src/
├── main.cpp                      # Application entry point
├── audio/
│   ├── audio_engine.cpp          # WASAPI audio backend
│   ├── audio_buffer.cpp          # Circular buffer implementation
│   ├── plugin_manager.cpp        # LV2 plugin loading and management
│   ├── audio_processing_chain.cpp # Plugin chain and routing
│   └── midi_handler.cpp          # Windows MIDI API integration
├── ui/
│   ├── main_window.cpp           # Main application window
│   ├── plugin_browser.cpp        # Plugin browser tree view
│   ├── active_plugins_panel.cpp  # Active plugins with inline controls
│   ├── audio_settings_dialog.cpp # Audio device/format configuration
│   └── plugin_parameters_window.cpp # Floating parameters window
├── core/
│   ├── config_manager.cpp        # Settings persistence
│   └── utils.cpp                 # Utility functions
└── platform/
    └── windows_api.cpp           # Windows API wrappers
```

## Documentation

- [Project Outline](PROJECT_OUTLINE.md) - Detailed development plan and timeline
- [Plugin Browser](docs/plugin_browser.md) - Plugin browser implementation details
- [Active Plugins Panel](docs/active_plugins_panel.md) - Active plugins panel documentation
- [Plugin Parameters Window](docs/plugin_parameters_window.md) - Parameters window guide

## License

MIT License - see [LICENSE](LICENSE) file for details.

### Important Note About Plugin Licenses

**Violet is an LV2 plugin host. LV2 plugins are separate software with their own licenses.**

Each LV2 plugin you use with Violet:
- Has its own copyright holder(s) and license terms
- May be GPL, LGPL, MIT, BSD, Apache, proprietary, or other licenses
- Is the user's responsibility to obtain legally and use compliantly
- Is NOT covered by Violet's MIT license

Before using any plugin:
1. Check the plugin's license terms
2. Ensure you have the right to use it (especially for commercial use)
3. Comply with the plugin's license requirements

Violet makes no warranties about third-party plugins. See [LICENSE](LICENSE) for complete details.
