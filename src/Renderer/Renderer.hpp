#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <vulkan/vulkan.h>
#include <vector>
#include <map>
#include <iostream>
#include <optional>

namespace rend
{
  struct QueueFamilyIndices
  {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> transferFamily;
    std::optional<uint32_t> computeFamily;

    bool isComplete()
    {
      return graphicsFamily.has_value() && transferFamily.has_value() && computeFamily.has_value();
    }
  };
  class Renderer
  {
  public:
    static Renderer& getInstance()
    {
      static Renderer instance;
      return instance;
    }
    
    void Init()
    {
      createVulkanInstance();
      createLogicalDevice();

    }
    
    void Terminate()
    {
      vkDestroyDevice(mDevice, nullptr);
      vkDestroyInstance(mInstance, nullptr);
    }
  private:
    void createVulkanInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  private:
    VkInstance mInstance;
    VkPhysicalDevice mPhysicalDevice;
    VkDevice mDevice;
  
    VkQueue mGraphicsQueue;
    VkQueue mComputeQueue;
    VkQueue mTransferQueue;
  };
}

#endif
