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

void MetalContext::fillCameraParams(const Camera &c, float *p, int offset) {
  cv::Mat mtx = c.getCameraMatrix();
  p[offset + 0] = (float)mtx.at<double>(0, 0);
  p[offset + 1] = (float)mtx.at<double>(1, 1);
  p[offset + 2] = (float)mtx.at<double>(0, 2);
  p[offset + 3] = (float)mtx.at<double>(1, 2);

  cv::Mat dist = c.getDistCoeffs();
  p[offset + 4] = (float)dist.at<double>(0, 0);
  p[offset + 5] = (float)dist.at<double>(0, 1);
  p[offset + 6] = (float)dist.at<double>(0, 2);
  p[offset + 7] = (float)dist.at<double>(0, 3);
  p[offset + 8] = dist.cols > 4 ? (float)dist.at<double>(0, 4) : 0.0f;

  Eigen::Matrix3d r = c.getBasis().transpose();
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      p[offset + 9 + i * 3 + j] = (float)r(i, j);
    }
  }

  Eigen::Vector3d pos = c.getPosition();
  p[offset + 18] = (float)pos.x();
  p[offset + 19] = (float)pos.y();
  p[offset + 20] = (float)pos.z();
}

bool MetalContext::executeRayIntersection(
    const std::vector<Match> &matches, const Camera &camera,
    const Camera &projector, float zMin, float zMax,
    std::vector<Eigen::Vector3d> &outPoints) {
  if (!init())
    return false;

  size_t numMatches = matches.size();
  if (numMatches == 0)
    return true;

  // Allocate buffers
  MTL::Buffer *inBuffer =
      m_device->newBuffer(matches.data(), numMatches * sizeof(Match),
                          MTL::ResourceStorageModeShared);
  MTL::Buffer *outBuffer = m_device->newBuffer(numMatches * sizeof(PointOutput),
                                               MTL::ResourceStorageModeShared);

  float rParams[44];
  fillCameraParams(camera, rParams, 0);
  fillCameraParams(projector, rParams, 21);
  rParams[42] = zMin;
  rParams[43] = zMax;

  MTL::Buffer *paramsBuffer = m_device->newBuffer(
      rParams, sizeof(rParams), MTL::ResourceStorageModeShared);

  MTL::CommandBuffer *cmdBuf = m_commandQueue->commandBuffer();
  MTL::ComputeCommandEncoder *encoder = cmdBuf->computeCommandEncoder();

  encoder->setComputePipelineState(m_rayIntersectPipeline);
  encoder->setBuffer(inBuffer, 0, 0);
  encoder->setBuffer(outBuffer, 0, 1);
  encoder->setBuffer(paramsBuffer, 0, 2);

  MTL::Size gridSize = MTL::Size::Make(numMatches, 1, 1);
  NS::UInteger threadGroupSize =
      m_rayIntersectPipeline->maxTotalThreadsPerThreadgroup();
  if (threadGroupSize > numMatches)
    threadGroupSize = numMatches;
  MTL::Size tgSize = MTL::Size::Make(threadGroupSize, 1, 1);

  encoder->dispatchThreads(gridSize, tgSize);
  encoder->endEncoding();
  cmdBuf->commit();
  cmdBuf->waitUntilCompleted();

  PointOutput *outData = (PointOutput *)outBuffer->contents();
  outPoints.clear();
  outPoints.reserve(numMatches);
  for (size_t i = 0; i < numMatches; i++) {
    if (outData[i].valid) {
      outPoints.emplace_back(outData[i].point[0], outData[i].point[1],
                             outData[i].point[2]);
    }
  }

  inBuffer->release();
  outBuffer->release();
  paramsBuffer->release();

  return true;
}
