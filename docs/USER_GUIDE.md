# Violet User Guide

**Version 0.78** | LV2 Plugin Host for Windows

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Interface Overview](#interface-overview)
3. [Working with Plugins](#working-with-plugins)
4. [Audio Configuration](#audio-configuration)
5. [Session Management](#session-management)
6. [Performance Optimization](#performance-optimization)
7. [Keyboard Shortcuts](#keyboard-shortcuts)
8. [Troubleshooting](#troubleshooting)

---

## Getting Started

### Installation

#### Using the Installer (Recommended)

1. Download `violet-0.78-setup.exe` from the releases page
2. Run the installer with administrator privileges
3. Follow the installation wizard
4. Launch Violet from the Start Menu or desktop shortcut

The installer automatically:
- Installs to `C:\Program Files\Violet`
- Creates Start Menu and desktop shortcuts
- Sets up the `LV2_PATH` environment variable
- Registers in Windows Add/Remove Programs

#### Manual Installation

1. Extract `violet.exe` to any folder
2. Set the `LV2_PATH` environment variable to your LV2 plugins directory
3. Run `violet.exe`

### First Launch

When you first launch Violet:

1. **Audio Engine Auto-Start**: The audio engine starts automatically with your default Windows audio device
2. **Plugin Scanning**: Violet scans for LV2 plugins in:
   - `%LV2_PATH%` (if set)
   - `C:\Program Files\Common Files\LV2`
   - `%APPDATA%\LV2`
3. **Empty Chain**: You start with an empty plugin chain ready to build

If no audio devices are found, you'll see an error message. Go to **Audio → Audio Settings** to select your audio interface.

---

## Interface Overview

Violet's interface consists of three main areas:

### 1. Menu Bar (Top)

- **File**: Session management (New, Open, Save, Exit)
- **Edit**: Cut, Copy, Paste operations (planned)
- **Audio**: Audio engine controls and device settings
- **View**: Theme selection and panel visibility
- **Help**: Documentation and about dialog

### 2. Plugin Browser (Left Panel)

The plugin browser shows all available LV2 plugins in your system:

- **Search Box**: Type to filter plugins by name
- **Category Tree**: Plugins organized by type
  - Distortion
  - Delay
  - Reverb
  - Dynamics
  - Modulation
  - EQ/Filter
  - And more...
- **Plugin Count**: Shows total available plugins at the bottom

**Loading a Plugin:**
- **Double-click** a plugin name, OR
- **Drag** the plugin to the Active Plugins panel

### 3. Active Plugins Panel (Center/Right)

This panel shows your current plugin chain:

- **Plugin Cards**: Each loaded plugin appears as a card
- **Parameter Sliders**: Adjust plugin parameters in real-time
- **Bypass Button (B)**: Temporarily disable a plugin without removing it
- **Remove Button (X)**: Remove plugin from the chain
- **Remove All Plugins**: Button at the bottom to clear entire chain

**Signal Flow**: Audio flows from **top to bottom** through the plugin chain.

### 4. Status Bar (Bottom)

Displays real-time information:

- **Audio Status**: "Audio: Running" or "Audio: Stopped"
- **CPU Usage**: Percentage of CPU used for audio processing
- **Latency**: Round-trip audio latency in milliseconds
- **Plugin Count**: Number of active plugins

---

## Working with Plugins

### Loading Plugins

**Method 1: Double-Click**
1. Find your plugin in the browser
2. Double-click the plugin name
3. Plugin appears at the bottom of your chain

**Method 2: Drag and Drop**
1. Click and hold on a plugin in the browser
2. Drag it to the Active Plugins panel
3. Drop it to add to your chain

### Adjusting Parameters

Each plugin shows its available parameters as sliders:

1. **Hover** over a slider to see the parameter name and current value
2. **Click and drag** left/right to adjust the value
3. **Changes apply instantly** to the audio signal
4. **Value display** updates in real-time

**Parameter Types:**
- **Continuous**: Smooth values (e.g., gain, frequency)
- **Toggle**: On/Off switches (e.g., bypass, mute)
- **Integer**: Discrete values (e.g., number of stages)

### Bypassing Plugins

To temporarily disable a plugin without removing it:

1. Click the **B (Bypass)** button on the plugin card
2. The plugin stays in the chain but doesn't process audio
3. Click again to re-enable

**Use Cases:**
- A/B comparison with and without effect
- Temporary disable during recording
- CPU saving when not needed

### Removing Plugins

**Remove Single Plugin:**
- Click the **X** button on the plugin card

**Remove All Plugins:**
- Click **"Remove All Plugins"** button at the bottom of the panel
- Confirms before clearing the entire chain

### Reordering Plugins (Planned)

*This feature is coming soon. Currently, plugins are added to the bottom of the chain.*

---

## Audio Configuration

### Audio Settings Dialog

Access via **Audio → Audio Settings** or `Ctrl+,`

#### Input Device
- Select your audio input (microphone, line-in, etc.)
- Choose "Default" to use Windows default device
- Hot-swap supported (no restart needed)

#### Output Device
- Select your audio output (speakers, headphones, etc.)
- Choose "Default" to use Windows default device
- Hot-swap supported (no restart needed)

#### Sample Rate
Options: **44100 Hz, 48000 Hz, 96000 Hz, 192000 Hz**

- **44.1 kHz**: CD quality, lowest CPU usage
- **48 kHz**: Standard for video/pro audio
- **96 kHz**: High-resolution audio
- **192 kHz**: Ultra-high resolution (highest CPU usage)

**Recommendation**: Start with 48 kHz unless you need higher quality

#### Buffer Size
Options: **64, 128, 256, 512, 1024, 2048 samples**

**Trade-off**: Lower buffer = lower latency, higher CPU usage

| Buffer Size | Latency @ 48kHz | Use Case |
|-------------|-----------------|----------|
| 64 samples  | ~1.3 ms         | Live performance, real-time control |
| 128 samples | ~2.7 ms         | Recording with monitoring |
| 256 samples | ~5.3 ms         | General use, balanced |
| 512 samples | ~10.7 ms        | Mixing, post-production |
| 1024 samples| ~21.3 ms        | Heavy plugin chains |
| 2048 samples| ~42.7 ms        | Mastering, offline processing |

**Recommendation**: Start with 256 or 512 samples

#### Applying Changes

- Click **OK** to apply settings
- The audio engine restarts automatically if running
- Settings are saved to configuration file

---

## Session Management

### What is a Session?

A session contains:
- List of loaded plugins in order
- All plugin parameter values
- Audio routing configuration (future)

Sessions are saved as `.violet` files.

### Creating a New Session

**File → New Session** or `Ctrl+N`

- Clears current plugin chain
- Starts with empty chain
- Prompts to save current session if modified

### Opening a Session

**File → Open Session** or `Ctrl+O`

1. Browse to your `.violet` file
2. Click Open
3. Plugin chain loads with all parameters restored
4. Audio engine continues running

**Recent Sessions:**
- Access recently opened sessions from **File → Recent**
- Up to 10 most recent sessions shown

### Saving a Session

**File → Save Session** or `Ctrl+S`

- Saves to current file if already saved
- Prompts for filename if new session

**File → Save Session As...** or `Ctrl+Shift+S`

- Always prompts for new filename
- Use to create variations of existing sessions

**Best Practices:**
- Save sessions in a dedicated folder
- Use descriptive names: `guitar-chain.violet`, `vocal-processing.violet`
- Save before making major changes
- Create templates for common setups

---

## Performance Optimization

### Monitoring Performance

Watch the **Status Bar** for:
- **CPU Usage**: Should stay below 70% for stable performance
- **Latency**: Displayed round-trip latency
- **Dropouts**: If audio crackles, CPU is overloaded

### Reducing CPU Usage

1. **Increase Buffer Size**
   - Audio → Audio Settings
   - Try doubling current buffer size
   - Latency increases, but more stable

2. **Remove Unused Plugins**
   - Each plugin adds CPU overhead
   - Bypass doesn't reduce CPU (plugin still runs)
   - Remove plugins you're not using

3. **Lower Sample Rate**
   - 48 kHz instead of 96 kHz cuts CPU usage ~50%
   - Only use high sample rates if needed

4. **Simplify Plugin Chains**
   - Some plugins are CPU-intensive
   - Use lighter alternatives when possible
   - Freeze/render heavy effects (future feature)

### Reducing Latency

1. **Decrease Buffer Size**
   - Audio → Audio Settings
   - Try 128 or 64 samples
   - Watch for audio dropouts

2. **Use ASIO Drivers (Future)**
   - Currently using WASAPI
   - ASIO support planned for even lower latency

3. **Optimize PC**
   - Close other applications
   - Disable background tasks
   - Use dedicated audio interface

### Troubleshooting Audio Issues

**No Sound:**
- Check Audio → Audio Settings
- Verify correct input/output devices selected
- Ensure audio engine is running (Audio → Start)
- Check plugin bypass states

**Audio Crackles/Dropouts:**
- Increase buffer size
- Close other programs
- Reduce plugin count
- Lower sample rate

**High Latency:**
- Decrease buffer size
- Use dedicated audio interface
- Ensure WASAPI Exclusive mode (future)

---

## Keyboard Shortcuts

### File Operations
- `Ctrl+N` - New Session
- `Ctrl+O` - Open Session
- `Ctrl+S` - Save Session
- `Ctrl+Shift+S` - Save Session As
- `Ctrl+Q` - Quit Application

### Edit Operations (Planned)
- `Ctrl+X` - Cut
- `Ctrl+C` - Copy
- `Ctrl+V` - Paste

### Audio Control
- `Ctrl+,` - Audio Settings
- `F5` - Start Audio Engine
- `F6` - Stop Audio Engine

### View
- `F11` - Toggle Full Screen (planned)
- `Ctrl+B` - Toggle Plugin Browser (planned)

### Plugin Operations
- `Delete` - Remove Selected Plugin (planned)
- `Ctrl+B` - Bypass Selected Plugin (planned)

---

## Troubleshooting

### Common Issues

#### "Failed to Initialize Audio Engine"

**Causes:**
- No audio devices detected
- Audio device in use by another application
- Driver issues

**Solutions:**
1. Check Windows Sound settings
2. Close other audio applications
3. Restart Windows Audio service
4. Update audio drivers
5. Try different audio device in Audio Settings

#### "No LV2 Plugins Found"

**Causes:**
- LV2 plugins not installed
- `LV2_PATH` not set correctly

**Solutions:**
1. Install LV2 plugins (e.g., from guitarix, Calf, LSP)
2. Set `LV2_PATH` environment variable:
   - Right-click This PC → Properties → Advanced → Environment Variables
   - Add new system variable: `LV2_PATH = C:\Path\To\Your\LV2\Plugins`
3. Restart Violet

#### "Plugin Failed to Load"

**Causes:**
- Corrupted plugin
- Incompatible plugin
- Missing dependencies

**Solutions:**
1. Try different plugin
2. Reinstall the plugin bundle
3. Check plugin documentation for requirements
4. Report issue on GitHub with plugin name

#### "High CPU Usage"

See [Performance Optimization](#performance-optimization) section.

### Getting Help

**Documentation:**
- README.md - Quick start and building
- PROJECT_OUTLINE.md - Development roadmap
- This guide - Comprehensive usage

**Support:**
- GitHub Issues: https://github.com/djshaji/violet/issues
- Discussions: https://github.com/djshaji/violet/discussions

**Bug Reports:**
Include:
1. Violet version (Help → About)
2. Windows version
3. Audio device model
4. Steps to reproduce
5. Console output (if using violet-console.exe)

---

## Tips and Tricks

### Efficient Workflow

1. **Create Templates**
   - Save common plugin chains as sessions
   - Load template instead of rebuilding each time

2. **Use Search**
   - Quickly find plugins by typing in search box
   - Clear search with X button

3. **Monitor Performance**
   - Keep Status Bar visible
   - Watch CPU before adding heavy plugins

4. **Save Often**
   - Use Ctrl+S frequently
   - Create backup copies of important sessions

### Advanced Usage

1. **A/B Testing**
   - Duplicate session (Save As)
   - Try different plugin chains
   - Compare results

2. **CPU Management**
   - Process heavy effects last in chain
   - Use bypass instead of remove for quick comparisons
   - Save CPU-intensive chains for offline processing

3. **Audio Quality**
   - Higher sample rate = better quality, more CPU
   - Lower buffer size = lower latency, less stable
   - Find balance for your use case

---

## Appendix

### Supported LV2 Plugins

Violet supports standard LV2 plugins including:

**Effects:**
- Guitarix plugins (distortion, amp simulation)
- Calf Studio Gear
- LSP (Linux Studio Plugins)
- ZamAudio
- Invada Studio plugins
- MDA plugins

**Utilities:**
- Level meters
- Analyzers
- Signal generators

**Requirements:**
- LV2 format (`.lv2` bundle)
- Audio ports (input/output)
- Control ports (parameters)

### File Locations

**Windows Installation:**
- Program: `C:\Program Files\Violet\`
- Configuration: `%APPDATA%\Violet\config.ini`
- Sessions: User-defined (recommended: `Documents\Violet Sessions\`)
- Logs: `%LOCALAPPDATA%\Violet\logs\` (future)

**LV2 Plugin Paths:**
- System: `C:\Program Files\Common Files\LV2\`
- User: `%APPDATA%\LV2\`
- Custom: Set via `LV2_PATH` environment variable

### Technical Specifications

**Audio Engine:**
- Backend: WASAPI (Windows Audio Session API)
- Sample Rates: 44.1, 48, 96, 192 kHz
- Bit Depth: 32-bit float (internal)
- Buffer Sizes: 64 - 2048 samples
- Channels: Stereo (mono planned)

**Plugin Support:**
- Format: LV2 (LILV library)
- Features: URID mapping, state save/restore
- UI: Native parameter controls (custom UI planned)

**Performance:**
- Typical CPU: < 5% per plugin
- Latency: 5-50ms depending on buffer size
- Max Plugins: Limited by CPU, typically 20-50

---

## License

Violet is released under the MIT License. See [LICENSE](../LICENSE) file for details.

### Important: Plugin Licenses

**LV2 plugins are separate software with their own licenses.** Each plugin has its own copyright holder and license terms (GPL, LGPL, MIT, BSD, proprietary, etc.). 

**It is your responsibility to:**
- Obtain plugins legally
- Review each plugin's license
- Ensure you have rights to use plugins (especially commercially)
- Comply with plugin license terms

Violet developers are not responsible for third-party plugins or their licensing. See the [LICENSE](../LICENSE) file for complete information.

## Credits

**Development:** Violet Team  
**LV2 Support:** LILV library by David Robillard  
**Audio Backend:** Windows WASAPI  

---

*Last Updated: November 2025 | Version 0.78*
