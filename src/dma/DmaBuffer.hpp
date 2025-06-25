#pragma once
#include <cstddef>
#include <memory>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>

class DmaBuffer {
public:
    DmaBuffer(size_t size, size_t alignment = 4096);
    ~DmaBuffer();
    
    int fd() const { return fd_; }
    size_t size() const { return size_; }
    size_t alignment() const { return alignment_; }
    
    void sync(bool start = true);

private:
    void allocate();
    
    int fd_ = -1;
    size_t size_;
    size_t alignment_;
};

using DmaBufferPtr = std::shared_ptr<DmaBuffer>;

class DmaBufferFactory {
public:
    enum class AllocationPolicy {
        DEFAULT,
        ALIGNED_4K,
        ALIGNED_1M,
        DEFER_FREE
    };

    DmaBufferPtr createBuffer(size_t size, 
                              AllocationPolicy policy = AllocationPolicy::DEFAULT);

private:
    DmaBufferPtr createDeferredBuffer(size_t size);
};