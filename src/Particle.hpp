#ifndef PARTICLE_H
#define PARTICLE_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <vector>

namespace part
{
  struct Particle
  {
    glm::vec2 position;
  };
  
  class ParticleManager
  {
  public:
    static ParticleManager& getInstance()
    {
      static ParticleManager instance;
      return instance;
    }
    
    void init(VkDevice device, VmaAllocator allocator)
    {
      mDevice = device;
      mAllocator = allocator;

      createParticleBuffer();
      createParticleSetLayout();
      createDescriptorPool();
      createDescriptorSet();
      
    }
  private:
    void createParticleBuffer();
    VkPipeline createComputePipeline( VkShaderModule computeShaderModule, const std::vector<VkPushConstantRange>& pcRanges, const std::vector<VkDescriptorSetLayout>& setLayouts) const;

    void createParticleSetLayout();
    void createDescriptorPool();
    void createDescriptorSet();
    void createComputeShaderModule();

    VkBuffer mParticleBuffer;
    VmaAllocation mParticleAllocation;    

    static constexpr uint32_t mParticleCount = 100'000;

    VkDevice mDevice;
    VmaAllocator mAllocator;
    VkDescriptorSetLayout mSetLayout;
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSet mDescriptorSet;
    VkShaderModule computeShaderModule;
  };
}
#endif 
