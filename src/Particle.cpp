#include "Particle.hpp"
#include "Renderer/Shader.hpp"

#include <stdexcept>

namespace part
{
  void ParticleManager::createComputeShaderModule()
  {
    std::string shaderGLSL = shader::ShaderTool::readFile("src/shader/partile.comp");
    std::vector<uint32_t> shaderSource = shader::ShaderTool::optimizeSPIRV(shader::ShaderTool::GLSLtoSPIRV(shaderGLSL, EShLanguage::EShLangCompute));

    VkShaderModuleCreateInfo createInfo {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = static_cast<uint32_t>(shaderSource.size()),
      .pCode = shaderSource.data(),
    };

    if(vkCreateShaderModule(mDevice, &createInfo, nullptr, &mComputeShaderModule) != VK_SUCCESS)
      throw std::runtime_error("Failed to create compute shader module!");

  }

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
      .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      .module = computeShaderModule,
      .pName = "main"
    };
  
  
    VkComputePipelineCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = shaderStage,
      .layout = layout
    };

    VkPipeline computePipeline;
    if(vkCreateComputePipelines(mDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &computePipeline) != VK_SUCCESS)
      throw std::runtime_error("Failed to create compute pipeline!");

    return computePipeline;
  }
  void ParticleManager::createParticleSetLayout()
  {
    VkDescriptorSetLayoutBinding binding{ 
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, 
      .pImmutableSamplers = nullptr
    };

    VkDescriptorSetLayoutCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings = &binding,
    };

    if(vkCreateDescriptorSetLayout(mDevice, &createInfo, nullptr, &mSetLayout) != VK_SUCCESS)
      throw std::runtime_error("Failed to create compute set layout");
  }
  void ParticleManager::createDescriptorPool()
  {
    VkDescriptorPoolSize poolSize{
      .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1 
    };

    VkDescriptorPoolCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1,
      .poolSizeCount = 1,
      .pPoolSizes = &poolSize
    };

    if(vkCreateDescriptorPool(mDevice, &createInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
      throw std::runtime_error("Failed to create compute descriptor pool!");
  }
  void ParticleManager::createDescriptorSet()
  {
    VkDescriptorSetAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = mDescriptorPool,
      .descriptorSetCount = 1,
      .pSetLayouts = &mSetLayout,
    };

    if(vkAllocateDescriptorSets(mDevice, &allocInfo, &mDescriptorSet) != VK_SUCCESS)
      throw std::runtime_error("Failed to create compute descriptor set!");
  }
} 
