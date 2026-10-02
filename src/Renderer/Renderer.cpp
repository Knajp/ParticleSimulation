#include "Renderer.hpp"
#include <GLFW/glfw3.h>
#include <cassert>
#include <set>
#include <map>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace rend {
#ifdef DEBUG
bool enableValidationLayers = true;
#else
bool enableValidationLayers = false;
#endif

std::vector<const char *> instanceLayers = {

#ifdef DEBUG
    "VK_LAYER_KHRONOS_validation"
#endif
};

std::vector<const char *> instanceExtensions = {

#ifdef DEBUG
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
};

std::vector<const char *> deviceLayers = {};
std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, // hey, it's me! severity!
                                                    VkDebugUtilsMessageTypeFlagsEXT type,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* userData) 
{
  std::cerr << "[Vulkan]" << pCallbackData->pMessage << "\n";
  return VK_FALSE;
}

void Renderer::createDebugMessenger()
{
  VkDebugUtilsMessengerCreateInfoEXT createInfo{
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = 
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = 
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debugCallback
  };

  if(vkCreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger) != VK_SUCCESS)
    throw std::runtime_error("Failed to create debug utils messenger!");
}
void Renderer::createVulkanInstance() {
  volkInitialize();

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_API_VERSION_1_4;
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "None";

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  if (!glfwExtensions)
    throw std::runtime_error("Failed to get GLFW Vulkan extensions!");

  for (uint32_t i = 0; i < glfwExtensionCount; i++)
    instanceExtensions.push_back(glfwExtensions[i]);

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(instanceExtensions.size());
  createInfo.ppEnabledExtensionNames = instanceExtensions.data();
  createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
  createInfo.ppEnabledLayerNames = instanceLayers.data();

  vkCreateInstance(&createInfo, nullptr, &mInstance);

  volkLoadInstance(mInstance);
}
void Renderer::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

  std::multimap<uint32_t, VkPhysicalDevice> candidates{};

  assert(deviceCount > 0);
  assert(devices.size() > 0);

  for (const VkPhysicalDevice &device : devices) {
    uint32_t score = 0;
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    std::cout << "Evaluating Physical Device Capability: " << props.deviceName
              << ";\n";

    if (!features.geometryShader) {
      std::cout << "No geometry shader support\n";
      continue;
    }

    QueueFamilyIndices familyIndices = findQueueFamilies(device);
    if (!familyIndices.isComplete()) {
      std::cout << "Incomplete queue indices!\n";
      continue;
    }

    if(!checkDeviceExtensionSupport(device))
    {
      std::cout << "Device does not support requested extensions!\n";
      continue;
    }
 
    bool swapchainAdequate = false;
    SwapchainSupportDetails swapchainDetails = querySwapchainSupport(device);
    swapchainAdequate = !swapchainDetails.presentModes.empty() && !swapchainDetails.surfaceFormats.empty();
    
    if(!swapchainAdequate)
    {
      std::cerr << "No surface present modes or no surface formats were found.\n";
      continue;
    }

    const int discreteBoost = 1000;

    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
      score += discreteBoost;

    score += props.limits.maxImageDimension2D;

    candidates.insert(std::make_pair(score, device));
  }

  assert(candidates.size() > 0);

  if (candidates.rbegin()->first > 0)
    mPhysicalDevice = candidates.rbegin()->second;
  else
    throw(std::runtime_error("Failed to find a suitable GPU!"));
}

bool Renderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

  for(const auto& extension : extensions)
    requiredExtensions.erase(extension.extensionName);

  return requiredExtensions.empty();
}

void Renderer::createLogicalDevice() {

  VkPhysicalDeviceVulkan14Features supportedFeatures14{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES, .pNext = nullptr};
  VkPhysicalDeviceVulkan13Features supportedFeatures13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = &supportedFeatures14};
  VkPhysicalDeviceVulkan12Features supportedFeatures12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &supportedFeatures13};
  VkPhysicalDeviceFeatures2 supportedFeatures11{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &supportedFeatures12};
  vkGetPhysicalDeviceFeatures2(mPhysicalDevice, &supportedFeatures11);

  if(!supportedFeatures13.dynamicRendering || !supportedFeatures13.synchronization2 || !supportedFeatures12.timelineSemaphore)
    throw std::runtime_error("Physical device doesn't match application Vulkan 1.X feature requirements.");

  VkPhysicalDeviceVulkan14Features features14{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES, .pNext = nullptr};
  VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = &features14, .synchronization2 = VK_TRUE, .dynamicRendering = VK_TRUE};
  VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &features13, .timelineSemaphore = VK_TRUE};
  VkPhysicalDeviceFeatures2 features11{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &features12};

  QueueFamilyIndices familyIndices = findQueueFamilies(mPhysicalDevice);
  float queuePriorities = 1.0f;

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueIndices = {
      familyIndices.presentFamily.value(), familyIndices.graphicsFamily.value(),
      familyIndices.computeFamily.value(),
      familyIndices.transferFamily.value()};

  for (const auto &idx : uniqueQueueIndices) {
    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = idx,
        .queueCount = 1,
        .pQueuePriorities = &queuePriorities,
    };
    queueCreateInfos.push_back(queueCreateInfo);
  }

  
  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pNext = &features11;
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.enabledLayerCount = static_cast<uint32_t>(deviceLayers.size());
  createInfo.ppEnabledLayerNames = deviceLayers.data();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create logical device!");

  vkGetDeviceQueue(mDevice, familyIndices.graphicsFamily.value(), 0,
                   &mGraphicsQueue);
  vkGetDeviceQueue(mDevice, familyIndices.presentFamily.value(), 0,
                   &mPresentQueue);
  vkGetDeviceQueue(mDevice, familyIndices.computeFamily.value(), 0,
                   &mComputeQueue);
  vkGetDeviceQueue(mDevice, familyIndices.transferFamily.value(), 0,
                   &mTransferQueue);
}

void Renderer::createCommandBuffers()
{
  QueueFamilyIndices familyIndices = findQueueFamilies(mPhysicalDevice);

  VkCommandPoolCreateInfo poolInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = nullptr,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = familyIndices.graphicsFamily.value()
  };

  if(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS)
    throw std::runtime_error("Failed to create command pool.");

  VkCommandPoolCreateInfo transferPoolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .queueFamilyIndex = familyIndices.transferFamily.value()
  };

  if(vkCreateCommandPool(mDevice, &transferPoolCreateInfo, nullptr, &mTransferPool) != VK_SUCCESS)
    throw std::runtime_error("Failed to create command pool.");
 
  VkCommandBufferAllocateInfo allocInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = mCommandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = MAX_FRAMES_IN_FLIGHT
  };

  mCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  if(vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data()) != VK_SUCCESS)
    throw std::runtime_error("Failed to allocate command buffers!");

  VkCommandBufferAllocateInfo transferAllocInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = mTransferPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1
  };

  if(vkAllocateCommandBuffers(mDevice, &transferAllocInfo, &mTransferBuffer) != VK_SUCCESS)
    throw std::runtime_error("Failed to create transfer buffer!");
}

void Renderer::createUniformBuffer()
{
  VkBufferCreateInfo createInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .size = sizeof(UniformBufferObject),
    .usage = VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE 
  };

  VmaAllocationCreateInfo allocInfo{
    .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
  };

  if(vmaCreateBuffer(mVmaAllocator, &createInfo, &allocInfo, &mUniformBuffer, &mUniformAllocation, nullptr) != VK_SUCCESS)
    throw std::runtime_error("Failed to create uniform buffer!");


}

void Renderer::writeUniformBuffer()
{

  VkBufferCreateInfo stagingInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .size = sizeof(UniformBufferObject),
    .usage = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE 
  };

  VmaAllocationCreateInfo stagingAllocInfo{
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO,
    .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
  };

  VkBuffer stagingBuffer;
  VmaAllocation stagingAllocation;

  if(vmaCreateBuffer(mVmaAllocator, &stagingInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS)
    throw std::runtime_error("Failed to create staging buffer!");

  float aspectRatio = static_cast<float>(mSwapchainExtent.width) / static_cast<float>(mSwapchainExtent.height);

  UniformBufferObject ubo{
    .proj = glm::ortho(-aspectRatio, aspectRatio, 1.0f, -1.0f)
  };

  void* data;
  vmaMapMemory(mVmaAllocator, stagingAllocation, &data);
  
  memcpy(data, &ubo, sizeof(ubo));

  vmaUnmapMemory(mVmaAllocator, stagingAllocation);

  auto* cb = beginSingleTimeCommands();

  VkBufferCopy2 copyRegion{
    .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
    .srcOffset = 0,
    .dstOffset = 0,
    .size = sizeof(UniformBufferObject)
  };

  VkCopyBufferInfo2 copyInfo{
    .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
    .srcBuffer = stagingBuffer, 
    .dstBuffer = mUniformBuffer,
    .regionCount = 1,
    .pRegions = &copyRegion
  };

  vkCmdCopyBuffer2(cb, &copyInfo);

  endSingleTimeCommands(cb);
}

void Renderer::recreateSwapchain(GLFWwindow* window)
{
  vkDeviceWaitIdle(mDevice);

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  while(width == 0 || height == 0)
  {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }
  cleanupSwapchain();

  createSwapchain(window);
  createSwapchainImageViews();
}

void Renderer::cleanupSwapchain()
{

  for(auto* imageView : mSwapchainImageViews)
    vkDestroyImageView(mDevice, imageView, nullptr);
  mSwapchainImageViews.clear();

  vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
  mSwapchainImages.clear();
}
VkCommandBuffer Renderer::beginSingleTimeCommands()
{
  VkCommandBuffer cb;
  VkCommandBufferAllocateInfo allocInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = mCommandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };

  if(vkAllocateCommandBuffers(mDevice, &allocInfo, &cb) != VK_SUCCESS)
    throw std::runtime_error("Failed to create single time command buffer!");

  VkCommandBufferBeginInfo beginInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };

  vkBeginCommandBuffer(cb, &beginInfo);

  return cb;
}

void Renderer::endSingleTimeCommands(VkCommandBuffer cb)
{
  vkEndCommandBuffer(cb);

  VkCommandBufferSubmitInfo cbInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
    .commandBuffer = cb,
    .deviceMask = 0
  };

  VkSubmitInfo2 submitInfo{
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
    .pNext = nullptr,
    .flags = 0,
    .commandBufferInfoCount = 1,
    .pCommandBufferInfos = &cbInfo,
  };

  if(vkQueueSubmit2(mTransferQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    throw std::runtime_error("Failed to submit to transfer queue!");

  vkQueueWaitIdle(mTransferQueue);
}
void Renderer::drawStorageBuffer(VkBuffer buffer, uint32_t vertexCount) const
{

  vkCmdBindPipeline(mCommandBuffers[mCurrentFrameInFlight], VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

  VkDescriptorBufferInfo bufferInfo {
    .buffer = buffer,
    .offset = 0,
    .range = VK_WHOLE_SIZE 
  };
  VkDescriptorBufferInfo uniformBufferInfo{
    .buffer = mUniformBuffer,
    .offset = 0,
    .range = VK_WHOLE_SIZE 
  };

  VkWriteDescriptorSet write {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext = nullptr,
    .dstSet = mDescriptorSet,
    .dstBinding = 0,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .pBufferInfo = &bufferInfo
  };
  VkWriteDescriptorSet write2{
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext = nullptr, 
    .dstSet = mDescriptorSet,
    .dstBinding = 1,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .pBufferInfo = &uniformBufferInfo 
  };

  VkWriteDescriptorSet writes[] = {write, write2};

  vkUpdateDescriptorSets(mDevice, 2, writes, 0, nullptr);

  VkBindDescriptorSetsInfo bindInfo{
    .sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
    .pNext = nullptr,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .layout = mGraphicsPipelineLayout,
    .firstSet = 0,
    .descriptorSetCount = 1,
    .pDescriptorSets = &mDescriptorSet,
    .dynamicOffsetCount = 0,
    .pDynamicOffsets = nullptr
  };

  vkCmdBindDescriptorSets2(mCommandBuffers[mCurrentFrameInFlight], &bindInfo);

  vkCmdDraw(mCommandBuffers[mCurrentFrameInFlight], vertexCount, 1, 0, 0);
}

VkCommandBuffer Renderer::beginRecording()
{
  vkWaitForFences(mDevice, 1, &mInFlightFences[mCurrentFrameInFlight], VK_TRUE, UINT64_MAX);

  VkAcquireNextImageInfoKHR acquireInfo {
    .sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
    .pNext = nullptr,
    .swapchain = mSwapchain,
    .timeout = UINT64_MAX,  
    .semaphore = mImageAvailableSemaphores[mCurrentFrameInFlight],
    .fence = VK_NULL_HANDLE,
    .deviceMask = 1
  };

  VkResult result = vkAcquireNextImage2KHR(mDevice, &acquireInfo, &mCurrentImageIndex);
  if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR)
    throw std::runtime_error("Failed to acquire image!");

  vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrameInFlight]);

  vkResetCommandBuffer(mCommandBuffers[mCurrentFrameInFlight], 0);

  VkCommandBufferBeginInfo beginInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext = nullptr,
    .flags = 0,
    .pInheritanceInfo = nullptr 
  };

  vkBeginCommandBuffer(mCommandBuffers[mCurrentFrameInFlight], &beginInfo);

  VkImageMemoryBarrier2 barrier {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
    .srcAccessMask = VK_ACCESS_2_NONE,
    .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = mSwapchainImages[mCurrentImageIndex],
    .subresourceRange{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    }
  };
  
  VkDependencyInfo depInfo {
    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &barrier,
  };

  vkCmdPipelineBarrier2(mCommandBuffers[mCurrentFrameInFlight], &depInfo);
  VkViewport viewport{
    .x = 0,
    .y = 0,
    .width = static_cast<float>(mSwapchainExtent.width),
    .height = static_cast<float>(mSwapchainExtent.height),
    .minDepth = 0.0f, 
    .maxDepth = 1.0f
  };
  VkRect2D scissor{
    .offset = {0, 0},
    .extent = mSwapchainExtent
  };

  vkCmdSetViewport(mCommandBuffers[mCurrentFrameInFlight], 0, 1, &viewport);
  vkCmdSetScissor(mCommandBuffers[mCurrentFrameInFlight], 0, 1, &scissor);

  return mCommandBuffers[mCurrentFrameInFlight];
}

void Renderer::beginRendering()
{
  VkRenderingAttachmentInfo attInfo{
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .pNext = nullptr,
    .imageView = mSwapchainImageViews[mCurrentImageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .resolveMode = VK_RESOLVE_MODE_NONE,
    .resolveImageView = VK_NULL_HANDLE,
    .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .clearValue = {
      .color = {{0.0f, 0.0f, 0.0f, 1.0f}}
    }
  };

  VkRenderingInfo renderingInfo{
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .pNext = nullptr,
    .flags = 0,
    .renderArea = {
      .offset = {0,0},
      .extent = mSwapchainExtent
    },
    .layerCount = 1,
    .viewMask = 0,
    .colorAttachmentCount = 1,
    .pColorAttachments = &attInfo,
    .pDepthAttachment = nullptr,
    .pStencilAttachment = nullptr 
  };

  vkCmdBeginRendering(mCommandBuffers[mCurrentFrameInFlight], &renderingInfo);

}
void Renderer::endAndSubmit(GLFWwindow* window)
{
  vkCmdEndRendering(mCommandBuffers[mCurrentFrameInFlight]);

  VkImageMemoryBarrier2 barrier{
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
    .dstAccessMask = VK_ACCESS_2_NONE,
    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = mSwapchainImages[mCurrentImageIndex],
    .subresourceRange{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };

  VkDependencyInfo depInfo{
    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &barrier
  };

  vkCmdPipelineBarrier2(mCommandBuffers[mCurrentFrameInFlight], &depInfo);

  if(vkEndCommandBuffer(mCommandBuffers[mCurrentFrameInFlight]) != VK_SUCCESS)
    throw std::runtime_error("Failed to end command buffer!");

  VkCommandBufferSubmitInfo cbInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
    .commandBuffer = mCommandBuffers[mCurrentFrameInFlight],
  };

  VkSemaphoreSubmitInfo waitInfo {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
    .semaphore = mImageAvailableSemaphores[mCurrentFrameInFlight],
    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
  };

  VkSemaphoreSubmitInfo signalInfo {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
    .semaphore = mRenderFinishedSemaphores[mCurrentImageIndex],
    .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT
  };

  VkSubmitInfo2 submitInfo{
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
    .pNext = nullptr,
    .flags = 0,
    .waitSemaphoreInfoCount = 1,
    .pWaitSemaphoreInfos = &waitInfo,
    .commandBufferInfoCount = 1,
    .pCommandBufferInfos = &cbInfo,
    .signalSemaphoreInfoCount = 1,
    .pSignalSemaphoreInfos = &signalInfo 
  };

  if(vkQueueSubmit2(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurrentFrameInFlight]) != VK_SUCCESS)
    throw std::runtime_error("Failed to submit command buffer to graphics queue!");


  VkPresentInfoKHR presentInfo{
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pNext = nullptr,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &mRenderFinishedSemaphores[mCurrentImageIndex],
    .swapchainCount = 1,
    .pSwapchains = &mSwapchain,
    .pImageIndices = &mCurrentImageIndex,
  };

  VkResult result = vkQueuePresentKHR(mPresentQueue, &presentInfo);

  if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mFramebufferResized)
    {recreateSwapchain(window); mFramebufferResized = false;}
  else if(result != VK_SUCCESS)
    throw std::runtime_error("Failed to present");
  mCurrentFrameInFlight = (mCurrentFrameInFlight + 1) % MAX_FRAMES_IN_FLIGHT;
}
void Renderer::createSynchronizationResources()
{
  mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  mRenderFinishedSemaphores.resize(mSwapchainImages.size());
  mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO 
  };
  VkFenceCreateInfo fenceInfo{
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT 
  };
 
  for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    if(vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
       vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to create sync resources!");
  }

  for(int i = 0; i < mSwapchainImages.size(); i++)
    if(vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to create render finished semaphore!");
}
void Renderer::createGraphicsShaderModules()
{
  std::string vsource = shader::ShaderTool::readFile("src/shader/shader.vert");
  std::vector<uint32_t> vspirv= shader::ShaderTool::GLSLtoSPIRV(vsource, EShLangVertex);
  vspirv = shader::ShaderTool::optimizeSPIRV(vspirv);

  VkShaderModuleCreateInfo vCreateInfo{
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .codeSize = vspirv.size() * sizeof(uint32_t),
    .pCode = vspirv.data() 
  };

  if(vkCreateShaderModule(mDevice, &vCreateInfo, nullptr, &mVertexShaderModule) != VK_SUCCESS)
    throw std::runtime_error("Failed to create vertex shader module!");

  std::string fsource = shader::ShaderTool::readFile("src/shader/shader.frag");
  std::vector<uint32_t> fspirv = shader::ShaderTool::GLSLtoSPIRV(fsource, EShLangFragment);
  fspirv = shader::ShaderTool::optimizeSPIRV(fspirv);

  VkShaderModuleCreateInfo fCreateInfo {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .codeSize = fspirv.size() * sizeof(uint32_t),
    .pCode = fspirv.data()
  };

  if(vkCreateShaderModule(mDevice, &fCreateInfo, nullptr, &mFragmentShaderModule) != VK_SUCCESS)
    throw std::runtime_error("Failed to create fragment shader module!");
}
QueueFamilyIndices Renderer::findQueueFamilies(VkPhysicalDevice device) const {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties2> queueFamilyProperties(queueFamilyCount);
  for(auto& queueFamily : queueFamilyProperties)
    queueFamily.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;

  vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyCount, queueFamilyProperties.data());

  int i = 0;
  for (const auto &qfp : queueFamilyProperties) {
    if (qfp.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      indices.graphicsFamily = i;
    if (qfp.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
      indices.computeFamily = i;
    if (qfp.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
      indices.transferFamily = i;

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, static_cast<uint32_t>(i),
                                         mWindowSurface, &presentSupport);

    if (presentSupport)
      indices.presentFamily = i;
    i++;
  }

  return indices;
}

void Renderer::initializeVMA()
{
  VmaVulkanFunctions vmaFuncInfo{};
  VmaAllocatorCreateInfo vmaAllocInfo{
    .physicalDevice = mPhysicalDevice,
    .device = mDevice,
    .pVulkanFunctions = &vmaFuncInfo,
    .instance = mInstance,
    .vulkanApiVersion = VK_API_VERSION_1_4
  };

  vmaImportVulkanFunctionsFromVolk(&vmaAllocInfo, &vmaFuncInfo);

  if(vmaCreateAllocator(&vmaAllocInfo, &mVmaAllocator) != VK_SUCCESS)
    throw std::runtime_error("Failed to create VMA allocator");
}
SwapchainSupportDetails Renderer::querySwapchainSupport(VkPhysicalDevice device) const
{
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mWindowSurface, &details.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, mWindowSurface, &formatCount, nullptr);
  details.surfaceFormats.resize(formatCount);
  if(formatCount)
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, mWindowSurface, &formatCount, details.surfaceFormats.data());

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, mWindowSurface, &presentModeCount, nullptr);
  details.presentModes.resize(presentModeCount);
  if(presentModeCount)
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, mWindowSurface, &presentModeCount, details.presentModes.data());

  return details;
}

VkSurfaceFormatKHR Renderer::pickSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
  for(const auto& format : formats)
    if(format.format == VK_FORMAT_B8G8R8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return format;

  return formats[0];
}

VkPresentModeKHR Renderer::pickPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
  for(const auto& mode : presentModes)
    if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
      return mode;

  std::cout << "Mailbox present mode not found on device, defaulting to FIFO.\n";
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{
  if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    return capabilities.currentExtent;

  int width, height; // NOLINT
  glfwGetFramebufferSize(window, &width, &height);

  VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
  actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.height);
  actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

  return actualExtent;
}

void Renderer::createSwapchain(GLFWwindow* window)
{
  SwapchainSupportDetails swapchainSupport = querySwapchainSupport(mPhysicalDevice);

  mSwapchainExtent = chooseSwapchainExtent(swapchainSupport.capabilities, window);
  mSwapchainFormat = pickSurfaceFormat(swapchainSupport.surfaceFormats);
  mSwapchainPresentMode = pickPresentMode(swapchainSupport.presentModes);

  uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
  if(swapchainSupport.capabilities.maxImageCount > 0 && swapchainSupport.capabilities.maxImageCount < imageCount)
    imageCount = swapchainSupport.capabilities.maxImageCount;

  QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  uint32_t queueFamilyCount = 0;
  uint32_t* queueIndicesPtr = nullptr;

  if(indices.graphicsFamily.value() != indices.presentFamily.value())
  {
    imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    queueFamilyCount = 2;
    queueIndicesPtr = queueFamilyIndices;
  }

  
  VkSwapchainCreateInfoKHR createInfo{
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext = nullptr,
    .flags = 0,
    .surface = mWindowSurface,
    .minImageCount = imageCount,
    .imageFormat = mSwapchainFormat.format,
    .imageColorSpace = mSwapchainFormat.colorSpace,
    .imageExtent = mSwapchainExtent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
    .imageSharingMode = imageSharingMode,
    .queueFamilyIndexCount = queueFamilyCount,
    .pQueueFamilyIndices = queueIndicesPtr,
    .preTransform = swapchainSupport.capabilities.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = mSwapchainPresentMode,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE
  };

  if(vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapchain) != VK_SUCCESS)
    throw std::runtime_error("Failed to create swapchain.");

  vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, nullptr);
  mSwapchainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, mSwapchainImages.data());

  mSwapchainImageViews.resize(mSwapchainImages.size());
  for(int i = 0; i < mSwapchainImageViews.size(); i++)
  {
    VkImageViewCreateInfo imageViewCreateInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = mSwapchainImages[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = mSwapchainFormat.format,
      .components{
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY 
      },
      .subresourceRange{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1 
      }
    };
    if(vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mSwapchainImageViews[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to create swapchain image view!");
  }
  VkImageCreateInfo depthCreateInfo{
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = depthFormat,
    .extent = {.width = mSwapchainExtent.width, .height = mSwapchainExtent.height, .depth =1},
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED 
  };

  VmaAllocationCreateInfo imageAllocInfo{
    .flags  = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO 
  };

  if(vmaCreateImage(mVmaAllocator, &depthCreateInfo, &imageAllocInfo, &mDepthImage, &mDepthImageAllocation, nullptr) != VK_SUCCESS)
    throw std::runtime_error("Failed to create depth image!");

  VkImageViewCreateInfo imageViewCreateInfo {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = mDepthImage,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = depthFormat,
    .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1}
  };

  if(vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mDepthImageView) != VK_SUCCESS)
    throw std::runtime_error("Failed to create depth image view.");

  
}
void Renderer::createDescriptorPool()
{
  VkDescriptorPoolSize poolSize{
    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1
  };

  VkDescriptorPoolSize uniformPoolSize{
    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1 
  };

  VkDescriptorPoolSize poolSizes[] = {poolSize, uniformPoolSize};

  VkDescriptorPoolCreateInfo createInfo{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .pNext = (void*)0,
    .flags = 0,
    .maxSets = 1, 
    .poolSizeCount = 2, 
    .pPoolSizes = poolSizes
  };

  if(vkCreateDescriptorPool(mDevice, &createInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
    throw std::runtime_error("Failed to create descriptor pool!");
}
void Renderer::createDescriptorSetLayout()
{
  VkDescriptorSetLayoutBinding binding {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, 
    .pImmutableSamplers = nullptr,
  };

  VkDescriptorSetLayoutBinding uniformBinding {
    .binding = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1 ,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };

  VkDescriptorSetLayoutBinding bindings[] = {binding, uniformBinding};

  VkDescriptorSetLayoutCreateInfo createInfo{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .bindingCount = 2,
    .pBindings = bindings
  };

  if(vkCreateDescriptorSetLayout(mDevice, &createInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS)
    throw std::runtime_error("Failed to create descriptor set layout!");
}
void Renderer::createDescriptorSets()
{
  VkDescriptorSetAllocateInfo allocInfo{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext = nullptr,
    .descriptorPool = mDescriptorPool,
    .descriptorSetCount = 1,
    .pSetLayouts = &mDescriptorSetLayout
  };

  if(vkAllocateDescriptorSets(mDevice, &allocInfo, &mDescriptorSet) != VK_SUCCESS)
    throw std::runtime_error("Failed to create descriptor set!");
}
void Renderer::createSwapchainImageViews()
{
  mSwapchainImageViews.resize(mSwapchainImages.size());

  for(int i = 0; i < mSwapchainImageViews.size(); i++)
  {  
    VkImageViewCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = mSwapchainImages[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = mSwapchainFormat.format,
      .components = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY 
      },
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };
    if(vkCreateImageView(mDevice, &createInfo, nullptr, &mSwapchainImageViews[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to create swapchain image view!");
  }
}
void Renderer::createWindowSurface(GLFWwindow *window) {
  if (glfwCreateWindowSurface(mInstance, window, nullptr, &mWindowSurface) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create GLFW window surface!");
}
} // namespace rend
