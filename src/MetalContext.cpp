#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include "MetalContext.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

MetalContext::MetalContext()
    : m_device(nullptr), m_commandQueue(nullptr), m_library(nullptr),
      m_rayIntersectPipeline(nullptr) {}

MetalContext::~MetalContext() { cleanup(); }

MetalContext &MetalContext::getInstance() {
  static MetalContext instance;
  return instance;
}

bool MetalContext::init() {
  if (m_device)
    return true; // Already initialized

  m_device = MTL::CreateSystemDefaultDevice();
  if (!m_device) {
    std::cerr << "Failed to find the default Metal device." << std::endl;
    return false;
  }

  m_commandQueue = m_device->newCommandQueue();
  if (!m_commandQueue) {
    std::cerr << "Failed to create Metal command queue." << std::endl;
    return false;
  }

  std::ifstream t("../src/kernels.metal");
  std::stringstream buffer;
  if (t.is_open()) {
    buffer << t.rdbuf();
  } else {
    std::ifstream t2("src/kernels.metal");
    if (t2.is_open()) {
      buffer << t2.rdbuf();
    } else {
      std::cerr << "Failed to find kernels.metal" << std::endl;
      return false;
    }
  }
  std::string shaderSourceStr = buffer.str();
  const char *shaderSource = shaderSourceStr.c_str();

  NS::Error *error = nullptr;
  auto sourceString = NS::String::string(shaderSource, NS::UTF8StringEncoding);
  MTL::CompileOptions *compileOptions = MTL::CompileOptions::alloc()->init();

  m_library = m_device->newLibrary(sourceString, compileOptions, &error);
  compileOptions->release();

  if (!m_library) {
    std::cerr << "Failed to compile Metal library from source." << std::endl;
    if (error) {
      std::cerr << "Error: " << error->localizedDescription()->utf8String()
                << std::endl;
    }
    return false;
  }

  auto functionName =
      NS::String::string("intersect_rays", NS::UTF8StringEncoding);
  MTL::Function *rayIntersectFn = m_library->newFunction(functionName);
  if (!rayIntersectFn) {
    std::cerr << "Failed to find 'intersect_rays' function in library."
              << std::endl;
    return false;
  }

  m_rayIntersectPipeline =
      m_device->newComputePipelineState(rayIntersectFn, &error);
  rayIntersectFn->release();

  if (!m_rayIntersectPipeline) {
    std::cerr << "Failed to create compute pipeline state for intersect_rays."
              << std::endl;
    if (error) {
      std::cerr << "Error: " << error->localizedDescription()->utf8String()
                << std::endl;
    }
    return false;
  }

  return true;
}

void MetalContext::cleanup() {
  if (m_rayIntersectPipeline) {
    m_rayIntersectPipeline->release();
    m_rayIntersectPipeline = nullptr;
  }
  if (m_library) {
    m_library->release();
    m_library = nullptr;
  }
  if (m_commandQueue) {
    m_commandQueue->release();
    m_commandQueue = nullptr;
  }
  if (m_device) {
    m_device->release();
    m_device = nullptr;
  }
}
