#ifndef RENDERER_HPP
#define RENDERER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
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
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
      return graphicsFamily.has_value() && transferFamily.has_value() && computeFamily.has_value() && presentFamily.has_value();
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
    
    void Init(GLFWwindow* window)
    {
      createVulkanInstance();
      createWindowSurface(window);
      pickPhysicalDevice();
      createLogicalDevice();
      
    }
    
    void Terminate()
    {
      vkDestroySurfaceKHR(mInstance, mWindowSurface, nullptr);
      vkDestroyDevice(mDevice, nullptr);
      vkDestroyInstance(mInstance, nullptr);
    }
  private:
    void createVulkanInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void createWindowSurface(GLFWwindow* window);
    
  private:
    VkInstance mInstance;
    VkPhysicalDevice mPhysicalDevice;
    VkDevice mDevice;
  
    VkQueue mGraphicsQueue;
    VkQueue mComputeQueue;
    VkQueue mTransferQueue;
    VkQueue mPresentQueue;

    VkSurfaceKHR mWindowSurface;
  };
}

#endif
