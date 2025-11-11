# Violet - LV2 Plugin Host

A lightweight LV2 plugin host for real-time audio processing on Windows.

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
  - Real-time sliders with value display
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

## Running

### Requirements
- Windows 10 or later
- LV2 plugins installed (the application will scan standard LV2 paths)

### Quick Start

1. Copy the executable to a Windows machine
2. Place LV2 plugins in a directory
3. Set `LV2_PATH` environment variable or run from plugin directory
4. Launch `violet.exe`

### Using the Application

1. **Audio Engine**: Automatically starts on launch with default audio device
2. **Browse Plugins**: Use the left panel to search and browse available LV2 plugins
3. **Load Plugins**: 
   - Double-click a plugin in the browser to add it to the chain
   - **OR drag-and-drop** a plugin from the browser to the active panel
4. **Adjust Parameters**: Use inline sliders in the active plugins panel
   - Changes are applied in real-time to the audio
5. **Control Plugins**: 
   - Click bypass button to enable/disable a plugin
   - Click remove button to remove a plugin
   - Click "Remove All Plugins" to clear the entire chain
6. **Configure Audio**:
   - Audio → Audio Settings to select audio devices
   - Choose sample rate and buffer size for optimal performance
   - Changes applied immediately (engine restarts if running)
7. **Control Audio**:
   - Audio → Start to begin processing
   - Audio → Stop to pause processing
   - Monitor CPU usage and latency in status bar
8. **Save Your Work**:
   - File → Save Session to save your current plugin chain
   - File → Open Session to load a previously saved setup
   - Sessions include all plugins and their parameter values

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

MIT License - see LICENSE file for details.
