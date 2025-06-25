#include "DmaFrameBuffer.hpp"

DmaFrameBuffer::DmaFrameBuffer(DmaBufferPtr dmaBuffer, 
                               const libcamera::StreamConfiguration& config)
    : dmaBuffer_(dmaBuffer) {
    
    std::vector<libcamera::FrameBuffer::Plane> planes;
    planes.push_back({
        .fd = dmaBuffer->fd(),
        .offset = 0,
        .size = static_cast<unsigned int>(config.frameSize),
        .stride = static_cast<unsigned int>(config.stride)
    });

    frameBuffer_ = std::make_unique<libcamera::FrameBuffer>(planes);
}