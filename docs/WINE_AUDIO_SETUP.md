# Wine Audio Setup Guide for Violet

## Common Audio Errors

### "Failed to select default output device"

This error occurs when Wine cannot enumerate audio devices through WASAPI. The specific HRESULT code will help diagnose:

#### Error: `0x80040154` - REGDB_E_CLASSNOTREG
**Cause:** MMDeviceEnumerator class not registered  
**Solution:** Install winepulse driver

```bash
# Install Wine PulseAudio driver
winetricks sound=pulse

# Or manually enable in Wine config
winecfg
# Go to Audio tab, select "PulseAudio" driver
```

#### Error: `0x800401f0` - CO_E_NOTINITIALIZED  
**Cause:** COM not initialized  
**Solution:** Already handled in code - this shouldn't occur

#### Error: `0x8889000a` - E_NOTFOUND
**Cause:** No audio devices detected  
**Solution:** 
1. Check that PulseAudio is running on Linux host
```bash
pulseaudio --check
pulseaudio --start
```

2. Verify Wine can see audio devices
```bash
wine control
# Check Sound settings
```

## Recommended Wine Setup

### 1. Install Wine Staging (recommended)
Wine Staging has better WASAPI support:
```bash
# Fedora
sudo dnf install wine-staging

# Ubuntu/Debian  
sudo apt install wine-staging
```

### 2. Configure Audio Driver
```bash
# Set PulseAudio as default
winetricks sound=pulse

# Or set in registry
wine reg add "HKCU\\Software\\Wine\\Drivers" /v Audio /d pulse /f
```

### 3. Test Audio
```bash
# Run violet-console.exe to see detailed error messages
wine violet-console.exe
```

## Expected Output

### Successful Initialization:
```
Initializing audio engine...
WASAPI device enumerator created successfully
Audio engine initialized successfully
Selecting default output device...
Selected default output device: {0.0.0.00000000}.{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
Creating audio client for output device: {0.0.0.00000000}.{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
Audio engine starting with:
  Sample rate: 44100 Hz
  Channels: 2
  Bit depth: 32 bits
  Buffer size: 256 samples
```

### Failed Initialization:
The error messages will now show exactly what failed:
```
Initializing audio engine...
Failed to create device enumerator: 0x80040154
MMDeviceEnumerator class not registered (Wine may need winepulse)
Failed to initialize WASAPI
```

## Windows Native Issues

On native Windows, if you see "Failed to select default output device":

1. **Check Windows Audio Service**
   - Open Services (services.msc)
   - Ensure "Windows Audio" service is running
   - Restart if needed

2. **Audio Device Disabled**
   - Open Sound settings
   - Ensure your playback device is enabled and set as default

3. **Permissions Issue**
   - Run as Administrator
   - Check antivirus isn't blocking audio access

## Debugging

Run with console output:
```bash
# On Wine
wine violet-console.exe

# On Windows
violet-console.exe
```

The detailed error messages will show:
- HRESULT error codes (e.g., 0x80040154)
- Human-readable error descriptions
- Device selection progress
- Audio format configuration

## Alternative: DirectSound Fallback

If WASAPI continues to fail on Wine, the application could be modified to use DirectSound as a fallback audio backend, which has better Wine support. This would require implementing a secondary audio engine.
