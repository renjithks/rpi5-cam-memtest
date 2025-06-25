#include "DmaBuffer.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdexcept>

DmaBuffer::DmaBuffer(size_t size, size_t alignment) 
    : size_(size), alignment_(alignment) {
    allocate();
}

DmaBuffer::~DmaBuffer() {
    if (fd_ >= 0) close(fd_);
}

void DmaBuffer::allocate() {
    const char* heap_path = "/dev/dma_heap/linux,cma";
    int heap_fd = open(heap_path, O_RDWR | O_CLOEXEC);
    if (heap_fd < 0) {
        throw std::runtime_error("DMA heap open failed");
    }

    struct dma_heap_allocation_data alloc = {0};
    alloc.len = size_;
    alloc.fd_flags = O_RDWR | O_CLOEXEC;
    
    if (ioctl(heap_fd, DMA_HEAP_IOCTL_ALLOC, &alloc) < 0) {
        close(heap_fd);
        throw std::runtime_error("DMA allocation failed");
    }

    fd_ = alloc.fd;
    close(heap_fd);
}

void DmaBuffer::sync(bool start) {
    struct dma_buf_sync sync = {0};
    sync.flags = (start ? DMA_BUF_SYNC_START : DMA_BUF_SYNC_END) | 
                 DMA_BUF_SYNC_RW;
    if (ioctl(fd_, DMA_BUF_IOCTL_SYNC, &sync) < 0) {
        throw std::runtime_error("DMA sync failed");
    }
}

DmaBufferPtr DmaBufferFactory::createBuffer(size_t size, AllocationPolicy policy) {
    switch (policy) {
        case AllocationPolicy::ALIGNED_4K:
            return std::make_shared<DmaBuffer>(size, 4096);
        case AllocationPolicy::ALIGNED_1M:
            return std::make_shared<DmaBuffer>(size, 1048576);
        case AllocationPolicy::DEFER_FREE:
            return createDeferredBuffer(size);
        default:
            return std::make_shared<DmaBuffer>(size);
    }
}

DmaBufferPtr DmaBufferFactory::createDeferredBuffer(size_t size) {
    // Implementation would use DMA_HEAP_ALLOC_FLAG_DEFER_FREE
    return std::make_shared<DmaBuffer>(size);
}