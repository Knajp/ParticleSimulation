#include "Particle.hpp"

#include <stdexcept>

namespace part
{
  void ParticleManager::createParticleBuffer()
  {
    VkBufferCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = mParticleCount * sizeof(Particle),
      .usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      
    };
    
    VmaAllocationCreateInfo allocationCreateInfo{
      .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    };

    if(vmaCreateBuffer(mAllocator, &createInfo, &allocationCreateInfo, &mParticleBuffer, &mParticleAllocation, nullptr) != VK_SUCCESS)
      throw std::runtime_error("Failed to create particle buffer!");
    
  }
  VkPipeline ParticleManager::createComputePipeline(const VkShaderModule computeShaderModule, const std::vector<VkPushConstantRange>& pcRanges, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) const
  {
    VkPipelineLayoutCreateInfo layoutCreateInfo
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
      .pSetLayouts = descriptorSetLayouts.data(),
      .pushConstantRangeCount = static_cast<uint32_t>(pcRanges.size()),
      .pPushConstantRanges = pcRanges.data()
    };

    VkPipelineLayout layout;
    if(vkCreatePipelineLayout(mDevice, &layoutCreateInfo, nullptr, &layout) != VK_SUCCESS)
      throw std::runtime_error("Failed to create pipeline layout!");

    VkPipelineShaderStageCreateInfo shaderStage{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pName = "main",
      .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      .module = computeShaderModule 
    };
  
  
    VkComputePipelineCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .layout = layout,
      .stage = shaderStage,
    };

    VkPipeline computePipeline;
    if(vkCreateComputePipelines(mDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &computePipeline) != VK_SUCCESS)
      throw std::runtime_error("Failed to create compute pipeline!");

    return computePipeline;
  }
} 
