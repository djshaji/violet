# WASAPI Callback vs Polling Mode Fix

## Problem Description
On Windows, the Violet audio engine was experiencing issues where:
1. Input (microphone) was being activated in polling mode instead of callback mode
2. Test tone generation was being enabled even when callback mode should be working  
3. WASAPI callback mode was not working properly, causing fallback to polling mode

## Root Cause Analysis

The issue was in the audio thread implementation (`AudioEngine::AudioThreadProc()`). The key problems were:

### 1. Missing Mode Tracking
The audio engine was not tracking whether WASAPI was initialized in event callback mode or polling mode. The `useEventCallback` variable was local to the `CreateAudioClient()` method but the audio thread needed to know which mode was active.

### 2. Incorrect Event Handling
The audio thread was always calling `WaitForSingleObject(audioEvent_, 1000)` regardless of whether WASAPI was in callback or polling mode. In polling mode, WASAPI doesn't signal the event, so the thread was constantly waiting for timeouts and only processing audio every second.

### 3. Event Callback Fallback Logic
The code correctly tried event callback mode first, then fell back to polling mode if initialization failed, but the audio thread wasn't aware of which mode succeeded.

## Solution Implemented

### 1. Added Mode Tracking Variables
```cpp
// In AudioEngine header
bool inputEventCallbackMode_;
bool outputEventCallbackMode_;
```

### 2. Fixed Audio Thread Logic
```cpp
// In AudioThreadProc()
if (outputEventCallbackMode_) {
    // Event callback mode: wait for audio event
    DWORD waitResult = WaitForSingleObject(audioEvent_, 1000);
    // ... handle event
} else {
    // Polling mode: sleep for a short period and check buffers
    Sleep(5); // 5ms sleep for polling mode
}
```

### 3. Updated Mode Flag Setting
Modified `CreateAudioClient()` to set the appropriate mode flags when initializing input and output clients.

## Technical Details

### Event Callback Mode (Preferred)
- WASAPI signals `audioEvent_` when buffers need processing
- Provides lower latency (around 3-10ms)
- More efficient CPU usage
- Audio thread waits for events instead of polling

### Polling Mode (Fallback)
- Audio thread actively checks buffer status every few milliseconds
- Higher latency (10-20ms) 
- Higher CPU usage due to constant polling
- Used when event callback mode fails (common on some Wine configurations)

## Benefits of the Fix

1. **Proper Mode Detection**: Audio engine now correctly detects and uses the appropriate mode
2. **Reduced Latency**: When event callback mode works, it provides much lower latency
3. **Correct Fallback**: Polling mode now works properly when callback mode fails
4. **Reduced CPU Usage**: No more unnecessary timeout waits in polling mode
5. **Better Audio Performance**: More responsive audio processing in both modes

## Test Results Expected

After this fix:
- Event callback mode should work properly on supported Windows systems
- Polling mode should work efficiently when callback mode is not available
- Test tone generation should only activate when microphone input truly fails
- Audio processing should be more responsive and use less CPU

The fix ensures that Violet works optimally in both WASAPI modes, automatically using the best available mode for the system configuration.