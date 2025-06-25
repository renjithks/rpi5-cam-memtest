#pragma once
#include "DmaBuffer.h"
#include <libcamera/libcamera.h>

class DmaFrameBuffer {
public:
    DmaFrameBuffer(DmaBufferPtr dmaBuffer, 
                   const libcamera::StreamConfiguration& config);
    
    libcamera::FrameBuffer* get() const { return frameBuffer_.get(); }
    DmaBufferPtr dmaBuffer() const { return dmaBuffer_; }

private:
    DmaBufferPtr dmaBuffer_;
    std::unique_ptr<libcamera::FrameBuffer> frameBuffer_;
};