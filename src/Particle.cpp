#include "Particle.hpp"
#include "Renderer/Shader.hpp"

#include <stdexcept>
#include <chrono>

namespace part
{
  void ParticleManager::createParticlePushConstantRange()
  {
    VkPushConstantRange pcRange{
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, 
      .offset = 0,
      .size = sizeof(pushConstants)
    };

    mParticlePCRange = pcRange;
  }
  void ParticleManager::createComputeShaderModule()
  {
    std::string shaderGLSL = shader::ShaderTool::readFile("src/shader/particle.comp");
    std::vector<uint32_t> shaderSource = shader::ShaderTool::optimizeSPIRV(shader::ShaderTool::GLSLtoSPIRV(shaderGLSL, EShLanguage::EShLangCompute));

    VkShaderModuleCreateInfo createInfo {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = static_cast<uint32_t>(shaderSource.size()) * sizeof(uint32_t),
      .pCode = shaderSource.data(),
    };

    if(vkCreateShaderModule(mDevice, &createInfo, nullptr, &mComputeShaderModule) != VK_SUCCESS)
      throw std::runtime_error("Failed to create compute shader module!");

  }

  void ParticleManager::transferIntoVertex(VkCommandBuffer cb)
  { 
    
  VkBufferMemoryBarrier2 barrier {
    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
    .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
    .srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, 
    .dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT, 
    .dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
    .buffer = mParticleBuffer,
    .offset = 0,
    .size = VK_WHOLE_SIZE 
  };

  VkDependencyInfo dependency{
    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .bufferMemoryBarrierCount = 1,
    .pBufferMemoryBarriers = &barrier 
  };

  vkCmdPipelineBarrier2(cb, &dependency);
  }

  void ParticleManager::createNextParticleBuffer()
  {
    VkBufferCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = mParticleCount * sizeof(int),
      .usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE 
    };
    
    VmaAllocationCreateInfo allocationCreateInfo{
      .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    };

    if(vmaCreateBuffer(mAllocator, &createInfo, &allocationCreateInfo, &mNextBuffer, &mNextAllocation, nullptr) != VK_SUCCESS)
      throw std::runtime_error("Failed to create next particle buffer!");
  }
  void ParticleManager::createCellHeadBuffer()
  {
    uint16_t gridHeight = 50, gridWidth = 50;

    VkBufferCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = gridHeight * gridWidth * sizeof(int),
      .usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE 
    };

    VmaAllocationCreateInfo allocationCreateInfo{
      .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    };

    if(vmaCreateBuffer(mAllocator, &createInfo, &allocationCreateInfo, &mCellHeadBuffer, &mCellHeadAllocation, nullptr) != VK_SUCCESS)
      throw std::runtime_error("Failed to create cell head buffer!");
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
  VkPipeline ParticleManager::createComputePipeline(const VkShaderModule computeShaderModule, const std::vector<VkPushConstantRange>& pcRanges, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, VkPipelineLayout& pipelineLayout) const
  {
    VkPipelineLayoutCreateInfo layoutCreateInfo
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
      .pSetLayouts = descriptorSetLayouts.data(),
      .pushConstantRangeCount = static_cast<uint32_t>(pcRanges.size()),
      .pPushConstantRanges = pcRanges.data()
    };

    if(vkCreatePipelineLayout(mDevice, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
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
      .layout = pipelineLayout
    };

    VkPipeline computePipeline;
    if(vkCreateComputePipelines(mDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &computePipeline) != VK_SUCCESS)
      throw std::runtime_error("Failed to create compute pipeline!");

    return computePipeline;
  }
  
  void ParticleManager::writeDescriptorSet()
  {
    VkDescriptorBufferInfo bufferInfo{
      .buffer = mParticleBuffer,
      .offset = 0,
      .range = VK_WHOLE_SIZE
    };

    VkWriteDescriptorSet write{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = mDescriptorSet,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &bufferInfo,
    };

    VkDescriptorBufferInfo cellHeadBufferInfo{
      .buffer = mCellHeadBuffer,
      .offset = 0,
      .range = VK_WHOLE_SIZE
    };

    VkWriteDescriptorSet write2{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = mDescriptorSet,
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &cellHeadBufferInfo 
    };
    
    VkDescriptorBufferInfo nextBufferInfo{
      .buffer = mNextBuffer,
      .offset = 0,
      .range = VK_WHOLE_SIZE 
    };

    VkWriteDescriptorSet write3{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = mDescriptorSet,
      .dstBinding = 2,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &nextBufferInfo 
    };

    VkWriteDescriptorSet writes[] = {write, write2, write3};
    vkUpdateDescriptorSets(mDevice, 3, writes, 0, nullptr);
  }

  void ParticleManager::createWaitFence()
  {
    VkFenceCreateInfo createInfo {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,

    };
    
    if(vkCreateFence(mDevice, &createInfo, nullptr, &mWaitFence) != VK_SUCCESS)
      throw std::runtime_error("Failed to create particle fence!");
  }
  void ParticleManager::createCommandBuffer(uint32_t computeFamilyIndex)
  {
    VkCommandPoolCreateInfo poolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .queueFamilyIndex = computeFamilyIndex 
    };
    
    if(vkCreateCommandPool(mDevice, &poolCreateInfo, nullptr, &mCommandPool) != VK_SUCCESS)
      throw std::runtime_error("Failed to create particle manager command pool");

    VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = mCommandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, 
      .commandBufferCount = 1,
    };

    if(vkAllocateCommandBuffers(mDevice, &allocInfo, &mCommandBuffer) != VK_SUCCESS)
      throw std::runtime_error("Failed to allocate particle manager command buffer");

  }
  void ParticleManager::invokeComputeShader(GLFWwindow* pWindow)
  { 
    int width, height; // NOLINT
    glfwGetFramebufferSize(pWindow, &width, &height);

    static auto previousTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    pushConstants pcValues {
      .init = mInit,
      ._padding = UINT32_MAX,
      .screenWidth = static_cast<float>(width),
      .screenHeight = static_cast<float>(height),
      .deltaTime = std::chrono::duration<float>(currentTime - previousTime).count()
    };
    previousTime = currentTime;

    if(mInit < 2) mInit++;

    VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };

    vkBeginCommandBuffer(mCommandBuffer, &beginInfo);

    vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline);
    
    vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipelineLayout, 0, 1, &mDescriptorSet, 0, nullptr);
    
    VkPushConstantsInfo pcInfo{
      .sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
      .layout = mComputePipelineLayout,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .offset = 0,
      .size = sizeof(pushConstants),
      .pValues = &pcValues
    };

    vkCmdPushConstants2(mCommandBuffer, &pcInfo);

    if(mInit == 0)
      vkCmdDispatch(mCommandBuffer, (100 * 100 + 256 - 1) / 256, 1, 1);
    else
      vkCmdDispatch(mCommandBuffer, (mParticleCount + 255) / 266, 1, 1); // NOLINT

    vkEndCommandBuffer(mCommandBuffer);

    VkCommandBufferSubmitInfo cbSubmit{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .commandBuffer = mCommandBuffer,
  
    };
    VkSubmitInfo2 submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .commandBufferInfoCount = 1,
      .pCommandBufferInfos = &cbSubmit,
    }; 

    vkQueueSubmit2(mQueue, 1, &submitInfo, mWaitFence);

    vkWaitForFences(mDevice, 1, &mWaitFence, VK_TRUE, UINT64_MAX);
    vkResetFences(mDevice, 1, &mWaitFence);
    vkResetCommandPool(mDevice, mCommandPool, 0);
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
    
    VkDescriptorSetLayoutBinding cellHeadBinding{
      .binding = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .pImmutableSamplers = nullptr 
    };
    
    VkDescriptorSetLayoutBinding nextBinding{
      .binding = 2,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .pImmutableSamplers = nullptr 
    };

    VkDescriptorSetLayoutBinding bindings[] = {binding, cellHeadBinding, nextBinding};
    VkDescriptorSetLayoutCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 3,
      .pBindings = bindings,
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
    
    VkDescriptorPoolSize cellHeadSize{
      .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1
    };

    VkDescriptorPoolSize nextSize{
      .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1 
    };

    VkDescriptorPoolSize poolSizes[] = {poolSize, cellHeadSize, nextSize};

    VkDescriptorPoolCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1,
      .poolSizeCount = 3,
      .pPoolSizes = poolSizes
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
