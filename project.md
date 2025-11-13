# Violet
LV2 Realtime Plugin Host for Windows
Violet is a lightweight LV2 plugin host designed for real-time audio processing on Windows. It provides a simple and efficient interface for loading and managing LV2 plugins, making it ideal for musicians and audio engineers who need a reliable tool for live performances and studio work.

## Features
- **Lightweight and Fast**: Optimized for low latency and minimal CPU usage.
- **User-Friendly Interface**: Intuitive design for easy navigation and plugin management.
- **Plugin Compatibility**: Supports a wide range of LV2 plugins.
- **Real-Time Processing**: Capable of handling real-time audio processing tasks.
- **Customizable Settings**: Allows users to adjust audio settings to suit their needs.
- **MIDI Support**: Integrates MIDI input for enhanced control over plugins.
- **Session Management**: Save and load sessions for quick setup.
- **Open Source**: Free to use and modify under the MIT License.

## Software stack
- **Programming Language**: C++
- **Audio Library**: LILV (LV2 host library)
- **GUI Framework**: Windows API with modern styling
- **Build System**: Meson
- **Compiler**: MinGW-w64 on Fedora Linux
- **Version Control**: Git
- **Audio Backend**: WASAPI

## User Interface
The user interface of Violet is designed to be clean and straightforward. It features a main window where users can load LV2 plugins, adjust parameters, and monitor audio levels. The interface includes:
- Top bar: Audio engine controls (start/stop, buffer size, sample rate), Application name, and Menu options.
- Main panel with two sections: 
  - Active plugins on the right: Displays the currently loaded plugin and its parameters.
  - Plugin Browser on the left: Easily browse and select installed LV2 plugins.
- Draggable plugin windows: Each loaded plugin opens in its own window for easy access and adjustment.
- Drag and drop support: Load plugins by dragging them into the main window.
- Parameter Controls: Sliders and knobs for adjusting plugin parameters.
- Status Bar: Displays real-time information about CPU usage, latency, and audio status.
- Theme Support: Light and dark mode options for user preference.
- Modern Windows styling: Consistent with Windows 10/11 design guidelines.

## Modern UI and Scaling

✅ **IMPLEMENTED** - Violet now features a modern, scalable user interface:

- **Modern Look**: 
  - ✅ Standard window with menu bar for full compatibility
  - ✅ Modern color palette with Light (white/purple) and Dark (dark gray/light purple) themes
  - ✅ Segoe UI font with ClearType antialiasing
  - ✅ Minimal, flat-styled controls with 4px rounded corners
  - ✅ Material Design-inspired color schemes
  
- **UI Scaling**: 
  - ✅ Full Per-Monitor DPI v2 awareness (Windows 10+)
  - ✅ Automatic scaling for all UI elements (fonts, icons, layouts)
  - ✅ Scales from 96 DPI to 384+ DPI displays
  - ✅ Seamless window resizing when moved between monitors
  - ✅ Backwards compatible with Windows 7/8
  
- **Custom Styling**: 
  - ✅ Modern flat buttons with hover states
  - ✅ Custom sliders with circular thumbs
  - ✅ Styled checkboxes and text inputs
  - ✅ Progress bars and combo boxes
  - ✅ Panels with optional elevation
  
- **Theme System**: 
  - ✅ Light and Dark themes
  - ✅ Cached GDI resources for performance
  - ✅ Theme switching API (`Theme::Instance().SetMode()`)
  - ✅ Menu-accessible theme switching (View > Theme)

**Technical Details:**
- **Color Schemes**: Material Design-inspired with purple primary (#BB86FC dark, #6200EE light)
- **Typography**: Segoe UI 11pt regular, 14pt semibold for titles
- **Window Style**: Standard window frame with visible menu bar
- **DPI Support**: Windows 10+ per-monitor v2, falls back to system DPI on older Windows

See [MODERN_UI.md](MODERN_UI.md) for complete implementation details and usage examples.

These improvements make Violet visually consistent with modern Windows 10/11 applications and ensure crisp, scalable UI on all display types including 4K and higher-resolution monitors.

## Audio Engine
Violet's audio engine is built on top of the WASAPI backend, ensuring low-latency audio processing. Key features of the audio engine include:
- Real-time audio processing capabilities.
- Support for multiple audio channels.
- Efficient CPU usage for smooth performance.
- Configurable buffer sizes and sample rates.
- Robust error handling to prevent audio dropouts.
- MIDI input support for enhanced plugin control.
- Session management for saving and loading audio setups.
- Compatibility with a wide range of LV2 plugins.
- Thread-safe processing to ensure stability during live performances.
- Integration with Windows audio settings for seamless operation.
- Support for audio routing and mixing between multiple plugins.
- Low-latency audio processing optimized for live performance scenarios.
- Real-time parameter automation for dynamic audio effects.
- Support for high-resolution audio formats (up to 192kHz).
- Efficient memory management to handle multiple plugins without performance degradation.
- Built-in metering for monitoring audio levels and CPU usage.
- Customizable audio settings to tailor performance to specific hardware configurations.
- Support for audio input/output device selection.
- Compatibility with ASIO drivers for professional audio interfaces.
- Real-time audio visualization for monitoring signal flow.
- Support for audio effects chaining and parallel processing.
- Integration with Windows audio session management for improved stability.
- Support for audio file playback and recording within the host.
- Advanced latency compensation to ensure tight synchronization between audio and MIDI.
- Real-time audio analysis tools for visual feedback on audio signals.
- Support for multi-core processing to leverage modern CPU architectures.
- Built-in support for common audio formats (WAV, AIFF, FLAC).
- Real-time audio monitoring with low latency.

