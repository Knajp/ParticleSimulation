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
    glm::vec2 screenSize;
    uint32_t padding;
  };
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
    
    void step(GLFWwindow* pWindow)
    {
      invokeComputeShader(pWindow);
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

    VkBuffer mParticleBuffer;
    VmaAllocation mParticleAllocation;    

    static constexpr uint32_t mParticleCount = 100'000;
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
