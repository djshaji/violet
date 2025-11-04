#include "violet/audio_buffer.h"
#include <algorithm>

namespace violet {

AudioBuffer::AudioBuffer(uint32_t channels, uint32_t capacity)
    : channels_(channels), capacity_(capacity) {
    Resize(channels, capacity);
}

AudioBuffer::~AudioBuffer() = default;

void AudioBuffer::Resize(uint32_t channels, uint32_t capacity) {
    channels_ = channels;
    capacity_ = capacity;
    
    channelBuffers_.clear();
    channelBuffers_.reserve(channels);
    
    for (uint32_t i = 0; i < channels; ++i) {
        channelBuffers_.emplace_back(std::make_unique<CircularBuffer<float>>(capacity));
    }
    
    // Resize temp buffer to handle the largest possible interleaved frame
    tempBuffer_.resize(channels * capacity);
}

size_t AudioBuffer::WriteInterleaved(const float* data, uint32_t frames) {
    if (channelBuffers_.empty() || !data) {
        return 0;
    }
    
    // Find the minimum available space across all channels
    size_t minAvailable = channelBuffers_[0]->AvailableWrite();
    for (uint32_t ch = 1; ch < channels_; ++ch) {
        minAvailable = std::min(minAvailable, channelBuffers_[ch]->AvailableWrite());
    }
    
    size_t framesToWrite = std::min(static_cast<size_t>(frames), minAvailable);
    if (framesToWrite == 0) {
        return 0;
    }
    
    // De-interleave and write to channel buffers
    for (uint32_t frame = 0; frame < framesToWrite; ++frame) {
        for (uint32_t ch = 0; ch < channels_; ++ch) {
            float sample = data[frame * channels_ + ch];
            channelBuffers_[ch]->Write(&sample, 1);
        }
    }
    
    return framesToWrite;
}

size_t AudioBuffer::ReadInterleaved(float* data, uint32_t frames) {
    if (channelBuffers_.empty() || !data) {
        return 0;
    }
    
    // Find the minimum available data across all channels
    size_t minAvailable = channelBuffers_[0]->AvailableRead();
    for (uint32_t ch = 1; ch < channels_; ++ch) {
        minAvailable = std::min(minAvailable, channelBuffers_[ch]->AvailableRead());
    }
    
    size_t framesToRead = std::min(static_cast<size_t>(frames), minAvailable);
    if (framesToRead == 0) {
        return 0;
    }
    
    // Read from channel buffers and interleave
    for (uint32_t frame = 0; frame < framesToRead; ++frame) {
        for (uint32_t ch = 0; ch < channels_; ++ch) {
            float sample = 0.0f;
            channelBuffers_[ch]->Read(&sample, 1);
            data[frame * channels_ + ch] = sample;
        }
    }
    
    return framesToRead;
}

size_t AudioBuffer::WriteChannel(uint32_t channel, const float* data, uint32_t frames) {
    if (channel >= channels_ || !data) {
        return 0;
    }
    
    return channelBuffers_[channel]->Write(data, frames);
}

size_t AudioBuffer::ReadChannel(uint32_t channel, float* data, uint32_t frames) {
    if (channel >= channels_ || !data) {
        return 0;
    }
    
    return channelBuffers_[channel]->Read(data, frames);
}

void AudioBuffer::Clear() {
    for (auto& buffer : channelBuffers_) {
        buffer->Clear();
    }
}

uint32_t AudioBuffer::GetAvailableFrames() const {
    if (channelBuffers_.empty()) {
        return 0;
    }
    
    // Return the minimum available frames across all channels
    size_t minAvailable = channelBuffers_[0]->AvailableRead();
    for (uint32_t ch = 1; ch < channels_; ++ch) {
        minAvailable = std::min(minAvailable, channelBuffers_[ch]->AvailableRead());
    }
    
    return static_cast<uint32_t>(minAvailable);
}

bool AudioBuffer::IsEmpty() const {
    if (channelBuffers_.empty()) {
        return true;
    }
    
    // Buffer is empty if all channels are empty
    for (const auto& buffer : channelBuffers_) {
        if (!buffer->IsEmpty()) {
            return false;
        }
    }
    return true;
}

bool AudioBuffer::IsFull() const {
    if (channelBuffers_.empty()) {
        return false;
    }
    
    // Buffer is full if any channel is full
    for (const auto& buffer : channelBuffers_) {
        if (buffer->IsFull()) {
            return true;
        }
    }
    return false;
}

CircularBuffer<float>* AudioBuffer::GetChannelBuffer(uint32_t channel) {
    if (channel >= channels_) {
        return nullptr;
    }
    return channelBuffers_[channel].get();
}

} // namespace violet