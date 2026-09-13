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
   
    VkPipelineRenderingCreateInfo rendering{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO, 
      .pNext = nullptr,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &mSwapchainFormat.format,
      .depthAttachmentFormat = depthFormat 
    };

    VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr, 
      .flags = 0,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = jointShaderModule, 
      .pName = "vert",
      .pSpecializationInfo = nullptr
    };

    VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = jointShaderModule, 
      .pName = "frag",
      .pSpecializationInfo = nullptr
    };
   
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineVertexInputStateCreateInfo vertexInput{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexBindingDescriptionCount = 0,
      .pVertexBindingDescriptions = nullptr,
      .vertexAttributeDescriptionCount = 0,
      .pVertexAttributeDescriptions = nullptr
    };

    VkPipelineRasterizationStateCreateInfo rasterizer{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL, .lineWidth = 1.0f 
    }; // NOLINT
  
    VkPipelineColorBlendAttachmentState colorAtt{
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable = VK_FALSE
    };

    VkPipelineColorBlendStateCreateInfo colorBlend{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .attachmentCount = 1,
      .pAttachments = &colorAtt,
      .logicOpEnable = VK_FALSE 
    };

    VkPipelineMultisampleStateCreateInfo multisampler{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE
    };

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamicStates
    };

    VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &rendering,
      .flags = 0,
      .stageCount = 2,
      .pStages = shaderStages,
      .pInputAssemblyState = &inputAssembly,
      .pVertexInputState = &vertexInput,
      .pRasterizationState = &rasterizer,
      .pColorBlendState = &colorBlend,
      .pMultisampleState = &multisampler,
      .pDynamicState = &dynamicState,
      .pViewportState = &viewportState
    };

    VkPipeline graphicsPipeline;
    if(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
      throw std::runtime_error("Failed to create graphics pipeline!");

    return graphicsPipeline;
  }


VkPipeline Renderer::createComputePipeline(const VkShaderModule computeShaderModule, const std::vector<VkPushConstantRange>& pcRanges, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) const
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
