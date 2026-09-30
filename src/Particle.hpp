#ifndef PARTICLE_H
#define PARTICLE_H

#include <glm/glm.hpp>
#include <volk/volk.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <GLFW/glfw3.h>
#include <vector>

namespace part
{
  struct Particle
  {
    glm::vec2 position;
    glm::vec3 color;
  };
 
  struct pushConstants
  {
    uint32_t init;
    uint32_t _padding;
    float screenWidth;
    float screenHeight;
  };
  static_assert(offsetof(pushConstants, screenWidth) == 8);
  static_assert(offsetof(pushConstants, screenHeight) == 12);
  static_assert(sizeof(pushConstants) == 16);

  class ParticleManager
  {
  public:
    static ParticleManager& getInstance()
    {
      static ParticleManager instance;
      return instance;
    }
    
    void init(VkDevice device, VmaAllocator allocator, VkQueue computeQueue, uint32_t computeFamilyIndex)
    {
      mDevice = device;
      mAllocator = allocator;
      mQueue = computeQueue;

      createParticleBuffer();
      createParticleSetLayout();
      createDescriptorPool();
      createDescriptorSet();
      createParticlePushConstantRange();
      createComputeShaderModule();
      mComputePipeline = createComputePipeline(mComputeShaderModule, {mParticlePCRange}, {mSetLayout}, mComputePipelineLayout);
      writeDescriptorSet();
      createCommandBuffer(computeFamilyIndex);
      createWaitFence();
    }
    
    void step(GLFWwindow* pWindow, VkCommandBuffer cb)
    {
      invokeComputeShader(pWindow);
      transferIntoVertex(cb);
    }
   
    VkBuffer getBufferHandle() const
    {
      return mParticleBuffer;
    }
    
    static uint32_t getParticleCount()
    {
      return mParticleCount;
    }
  private:
    void createParticleBuffer();
    VkPipeline createComputePipeline( VkShaderModule computeShaderModule, const std::vector<VkPushConstantRange>& pcRanges, const std::vector<VkDescriptorSetLayout>& setLayouts, VkPipelineLayout& pipelineLayout) const;

    void createParticleSetLayout();
    void createDescriptorPool();
    void createDescriptorSet();
    void createComputeShaderModule();
    void createParticlePushConstantRange();
    void writeDescriptorSet();
    void createWaitFence();
    
    void createCommandBuffer(uint32_t computeFamilyIndex);

    void invokeComputeShader(GLFWwindow* pWindow);
    void transferIntoVertex(VkCommandBuffer cb);

    VkBuffer mParticleBuffer;
    VmaAllocation mParticleAllocation;    

    static constexpr uint32_t mParticleCount = 1000;
    uint32_t mInit = 1;

    VkDevice mDevice;
    VmaAllocator mAllocator;
    VkDescriptorSetLayout mSetLayout;
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSet mDescriptorSet;
    VkShaderModule mComputeShaderModule;
    VkPipeline mComputePipeline;
    VkPipelineLayout mComputePipelineLayout;
    VkPushConstantRange mParticlePCRange;
    VkCommandPool mCommandPool;
    VkCommandBuffer mCommandBuffer;
    VkFence mWaitFence;
    VkQueue mQueue;
  };
}
#endif 
