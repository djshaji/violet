# Violet Quick Reference Card

**Version 0.78** | LV2 Plugin Host for Windows

---

## Quick Start

1. **Install**: Run `violet-0.78-setup.exe` with admin privileges
2. **Launch**: Start Menu → Violet
3. **Load Plugin**: Double-click plugin in left panel
4. **Adjust**: Use sliders to control parameters
5. **Save**: File → Save Session

---

## Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| New Session | `Ctrl+N` |
| Open Session | `Ctrl+O` |
| Save Session | `Ctrl+S` |
| Save Session As | `Ctrl+Shift+S` |
| Quit | `Ctrl+Q` |
| Audio Settings | `Ctrl+,` |
| Start Audio | `F5` |
| Stop Audio | `F6` |

---

## Interface Layout

```
┌─────────────────────────────────────────────────┐
│  File  Edit  Audio  View  Help       [Menu Bar]│
├──────────────┬──────────────────────────────────┤
│              │                                  │
│   Plugin     │      Active Plugins Panel        │
│   Browser    │                                  │
│              │  ┌─────────────────────────────┐ │
│  [Search]    │  │ Plugin 1        [B] [X]    │ │
│              │  │  Param: [====|-----] 50%   │ │
│  Categories: │  └─────────────────────────────┘ │
│  ├─Distortion│  ┌─────────────────────────────┐ │
│  ├─Delay     │  │ Plugin 2        [B] [X]    │ │
│  ├─Reverb    │  │  Param: [======|---] 70%   │ │
│  └─...       │  └─────────────────────────────┘ │
│              │                                  │
│  [Plugin 1]  │  [Remove All Plugins]            │
│  [Plugin 2]  │                                  │
│              │                                  │
├──────────────┴──────────────────────────────────┤
│  Audio: Running | CPU: 12% | Latency: 5.3ms    │
└─────────────────────────────────────────────────┘
```

---

## Loading Plugins

**Method 1: Double-Click**
- Double-click plugin name in browser

**Method 2: Drag & Drop**
- Drag plugin from browser to active panel

---

## Plugin Controls

| Button | Function |
|--------|----------|
| **B** | Bypass (disable temporarily) |
| **X** | Remove from chain |
| **Slider** | Adjust parameter value |

---

## Audio Settings

**Access**: Audio → Audio Settings

| Setting | Options | Recommendation |
|---------|---------|----------------|
| Sample Rate | 44.1, 48, 96, 192 kHz | 48 kHz |
| Buffer Size | 64 - 2048 samples | 256 samples |
| Input Device | Windows devices | Your interface |
| Output Device | Windows devices | Your interface |

---

## Buffer Size Guide

| Size | Latency @ 48kHz | Use Case |
|------|----------------|----------|
| 64   | ~1.3 ms | Live performance |
| 128  | ~2.7 ms | Recording |
| 256  | ~5.3 ms | General use |
| 512  | ~10.7 ms | Mixing |
| 1024 | ~21.3 ms | Heavy chains |
| 2048 | ~42.7 ms | Mastering |

---

## Troubleshooting

### No Sound
1. Check Audio → Audio Settings
2. Verify device selection
3. Check plugin bypass states
4. Restart audio engine (F6, F5)

### Crackling Audio
1. Increase buffer size
2. Close other programs
3. Remove some plugins
4. Lower sample rate

### High CPU
1. Increase buffer size
2. Remove unused plugins
3. Lower sample rate
4. Close background apps

### No Plugins Found
1. Install LV2 plugins
2. Set LV2_PATH environment variable
3. Restart Violet

---

## Performance Tips

✓ **Start with 256 buffer, 48kHz sample rate**  
✓ **Monitor CPU usage in status bar**  
✓ **Save sessions frequently (Ctrl+S)**  
✓ **Use bypass for A/B testing**  
✓ **Remove plugins instead of just bypassing**  
✓ **Close other applications when recording**

---

## Session Management

### Save Session
```
File → Save Session (Ctrl+S)
```
Saves: Plugin list + all parameter values

### Open Session
```
File → Open Session (Ctrl+O)
```
Restores: Complete plugin chain state

### Best Practices
- Save in dedicated folder
- Use descriptive names
- Save before major changes
- Create templates for common setups

---

## Common Workflows

### Guitar Processing Chain
1. Load amp simulator plugin
2. Add cabinet simulator
3. Add reverb or delay
4. Adjust parameters to taste
5. Save as `guitar-chain.violet`

### Vocal Processing
1. Load EQ plugin
2. Add compressor
3. Add reverb
4. Fine-tune parameters
5. Save as `vocal-processing.violet`

### Mastering Chain
1. Load multiband compressor
2. Add EQ for final touches
3. Add limiter last
4. Use 512+ buffer size
5. Save as `mastering.violet`

---

## Signal Flow

```
Input Device
     ↓
  Plugin 1 (top)
     ↓
  Plugin 2
     ↓
  Plugin 3
     ↓
  Plugin N (bottom)
     ↓
Output Device
```

Audio flows **top to bottom** through plugin chain.

---

## File Locations

**Program**: `C:\Program Files\Violet\`  
**Config**: `%APPDATA%\Violet\config.ini`  
**Sessions**: User-defined (`.violet` files)  
**Plugins**: `%LV2_PATH%` or `C:\Program Files\Common Files\LV2\`

---

## System Requirements

**Minimum:**
- Windows 10 64-bit
- 2 GB RAM
- Any audio interface

**Recommended:**
- Windows 11 64-bit
- 4+ GB RAM
- Dedicated audio interface (ASIO)
- Multi-core CPU

---

## Getting Help

**Documentation**: `docs/USER_GUIDE.md`  
**Issues**: github.com/djshaji/violet/issues  
**Discussions**: github.com/djshaji/violet/discussions

---

## Status Bar Icons

| Indicator | Meaning |
|-----------|---------|
| Audio: Running | Engine processing audio |
| Audio: Stopped | Engine paused |
| CPU: XX% | Processor usage |
| Latency: X.Xms | Round-trip delay |
| Plugins: N | Active plugin count |

---

**Print this card for quick reference!**

*Violet v0.78 | MIT License | github.com/djshaji/violet*
