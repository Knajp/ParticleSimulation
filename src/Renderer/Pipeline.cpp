#include "Renderer.hpp"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace rend
{
  VkPipeline Renderer::createGraphicsPipeline(VkShaderModule jointShaderModule) const
  {
    VkPipelineLayoutCreateInfo layoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = 0,
      .pSetLayouts = nullptr,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr
    };

    VkPipelineLayout pipelineLayout;
    if(vkCreatePipelineLayout(mDevice, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
      throw std::runtime_error("Failed to create graphics pipeline layout!");
    
    VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr, 
      .flags = 0,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .pName = "vert",
      .module = jointShaderModule 
    };

    VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .pName = "frag",
      .module = jointShaderModule 
    };
   
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .primitiveRestartEnable = VK_FALSE,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };
    
    VkPipelineVertexInputStateCreateInfo vertexInput{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexAttributeDescriptionCount = 0,
      .vertexBindingDescriptionCount = 0
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stageCount = 2,
      .pStages = shaderStages,
      .pInputAssemblyState = &inputAssembly,
      .pVertexInputState = &vertexInput
    };
  }
}
