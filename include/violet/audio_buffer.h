#pragma once

#include <vector>
#include <atomic>
#include <memory>
#include <cstring>

namespace violet {

// Thread-safe circular buffer for audio data
template<typename T>
class CircularBuffer {
public:
    explicit CircularBuffer(size_t capacity)
        : buffer_(capacity)
        , capacity_(capacity)
        , readPos_(0)
        , writePos_(0)
        , size_(0) {
    }
    
    // Write data to buffer (returns number of elements actually written)
    size_t Write(const T* data, size_t count) {
        size_t available = capacity_ - size_.load();
        size_t toWrite = std::min(count, available);
        
        if (toWrite == 0) {
            return 0;
        }
        
        size_t writePos = writePos_.load();
        
        // Handle wrap-around
        if (writePos + toWrite <= capacity_) {
            // No wrap-around needed
            std::memcpy(&buffer_[writePos], data, toWrite * sizeof(T));
        } else {
            // Wrap-around needed
            size_t firstPart = capacity_ - writePos;
            size_t secondPart = toWrite - firstPart;
            
            std::memcpy(&buffer_[writePos], data, firstPart * sizeof(T));
            std::memcpy(&buffer_[0], data + firstPart, secondPart * sizeof(T));
        }
        
        writePos_.store((writePos + toWrite) % capacity_);
        size_.fetch_add(toWrite);
        
        return toWrite;
    }
    
    // Read data from buffer (returns number of elements actually read)
    size_t Read(T* data, size_t count) {
        size_t available = size_.load();
        size_t toRead = std::min(count, available);
        
        if (toRead == 0) {
            return 0;
        }
        
        size_t readPos = readPos_.load();
        
        // Handle wrap-around
        if (readPos + toRead <= capacity_) {
            // No wrap-around needed
            std::memcpy(data, &buffer_[readPos], toRead * sizeof(T));
        } else {
            // Wrap-around needed
            size_t firstPart = capacity_ - readPos;
            size_t secondPart = toRead - firstPart;
            
            std::memcpy(data, &buffer_[readPos], firstPart * sizeof(T));
            std::memcpy(data + firstPart, &buffer_[0], secondPart * sizeof(T));
        }
        
        readPos_.store((readPos + toRead) % capacity_);
        size_.fetch_sub(toRead);
        
        return toRead;
    }
    
    // Peek at data without removing it
    size_t Peek(T* data, size_t count) const {
        size_t available = size_.load();
        size_t toPeek = std::min(count, available);
        
        if (toPeek == 0) {
            return 0;
        }
        
        size_t readPos = readPos_.load();
        
        // Handle wrap-around
        if (readPos + toPeek <= capacity_) {
            // No wrap-around needed
            std::memcpy(data, &buffer_[readPos], toPeek * sizeof(T));
        } else {
            // Wrap-around needed
            size_t firstPart = capacity_ - readPos;
            size_t secondPart = toPeek - firstPart;
            
            std::memcpy(data, &buffer_[readPos], firstPart * sizeof(T));
            std::memcpy(data + firstPart, &buffer_[0], secondPart * sizeof(T));
        }
        
        return toPeek;
    }
    
    // Skip data without reading it
    size_t Skip(size_t count) {
        size_t available = size_.load();
        size_t toSkip = std::min(count, available);
        
        if (toSkip == 0) {
            return 0;
        }
        
        size_t readPos = readPos_.load();
        readPos_.store((readPos + toSkip) % capacity_);
        size_.fetch_sub(toSkip);
        
        return toSkip;
    }
    
    // Clear the buffer
    void Clear() {
        readPos_.store(0);
        writePos_.store(0);
        size_.store(0);
    }
    
    // Get current size
    size_t Size() const {
        return size_.load();
    }
    
    // Get capacity
    size_t Capacity() const {
        return capacity_;
    }
    
    // Check if buffer is empty
    bool IsEmpty() const {
        return size_.load() == 0;
    }
    
    // Check if buffer is full
    bool IsFull() const {
        return size_.load() == capacity_;
    }
    
    // Get available space for writing
    size_t AvailableWrite() const {
        return capacity_ - size_.load();
    }
    
    // Get available data for reading
    size_t AvailableRead() const {
        return size_.load();
    }

private:
    std::vector<T> buffer_;
    const size_t capacity_;
    std::atomic<size_t> readPos_;
    std::atomic<size_t> writePos_;
    std::atomic<size_t> size_;
};

// Audio buffer manager for handling multiple channels and formats
class AudioBuffer {
public:
    AudioBuffer(uint32_t channels, uint32_t capacity);
    ~AudioBuffer();
    
    // Resize buffer
    void Resize(uint32_t channels, uint32_t capacity);
    
    // Write interleaved audio data
    size_t WriteInterleaved(const float* data, uint32_t frames);
    
    // Read interleaved audio data
    size_t ReadInterleaved(float* data, uint32_t frames);
    
    // Write/read specific channel
    size_t WriteChannel(uint32_t channel, const float* data, uint32_t frames);
    size_t ReadChannel(uint32_t channel, float* data, uint32_t frames);
    
    // Clear all channels
    void Clear();
    
    // Get buffer info
    uint32_t GetChannels() const { return channels_; }
    uint32_t GetCapacity() const { return capacity_; }
    uint32_t GetAvailableFrames() const;
    bool IsEmpty() const;
    bool IsFull() const;
    
    // Direct channel access (for advanced use)
    CircularBuffer<float>* GetChannelBuffer(uint32_t channel);
    
private:
    uint32_t channels_;
    uint32_t capacity_;
    std::vector<std::unique_ptr<CircularBuffer<float>>> channelBuffers_;
    std::vector<float> tempBuffer_; // For interleaved conversions
};

// Ring buffer specifically for MIDI events
struct MidiEvent {
    uint32_t timestamp;  // Sample timestamp
    uint8_t data[4];     // MIDI data (up to 4 bytes)
    uint8_t size;        // Number of valid bytes in data
    
    MidiEvent() : timestamp(0), size(0) {
        data[0] = data[1] = data[2] = data[3] = 0;
    }
    
    MidiEvent(uint32_t ts, const uint8_t* midiData, uint8_t dataSize)
        : timestamp(ts), size(std::min(dataSize, static_cast<uint8_t>(4))) {
        for (int i = 0; i < 4; ++i) {
            data[i] = (i < size) ? midiData[i] : 0;
        }
    }
};

using MidiBuffer = CircularBuffer<MidiEvent>;

} // namespace violet