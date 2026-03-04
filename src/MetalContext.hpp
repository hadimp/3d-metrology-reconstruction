#pragma once
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>

class MetalContext {
public:
    static MetalContext& getInstance();
    
    MTL::Device* getDevice() const { return m_device; }
    MTL::CommandQueue* getCommandQueue() const { return m_commandQueue; }
    MTL::ComputePipelineState* getRayIntersectionPipeline() const { return m_rayIntersectPipeline; }

    bool init();
    void cleanup();

private:
    MetalContext();
    ~MetalContext();
    
    MetalContext(const MetalContext&) = delete;
    MetalContext& operator=(const MetalContext&) = delete;

    MTL::Device* m_device;
    MTL::CommandQueue* m_commandQueue;
    MTL::Library* m_library;
    MTL::ComputePipelineState* m_rayIntersectPipeline;
};
