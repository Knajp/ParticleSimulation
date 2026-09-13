#ifndef RENDERER_HPP
#define RENDERER_HPP

#define GLFW_INCLUDE_VULKAN
#define VK_NO_PROTOTYPES
#include <GLFW/glfw3.h>
#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <optional>
#include <vector>
#include <limits>
#include <string>

namespace rend
{
  struct QueueFamilyIndices
  {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> transferFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const
    {
      return graphicsFamily.has_value() && transferFamily.has_value() && computeFamily.has_value() && presentFamily.has_value();
    }
  };

  struct SwapchainSupportDetails
  {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
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
      createSwapchain(window);      
      createComputeSetLayout();
      createDescriptorPool();
      createComputeDescriptorSet();
    }
   
    void Terminate()
    {
      vkDestroyDescriptorPool(mDevice, mComputeDescriptorPool, nullptr);
      vkDestroyDescriptorSetLayout(mDevice, mComputeSetLayout, nullptr);
      for(auto imageView : mSwapchainImageViews) // NOLINT
        vkDestroyImageView(mDevice, imageView, nullptr);
      vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
      vmaDestroyAllocator(mVmaAllocator);
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
    static bool checkDeviceExtensionSupport(VkPhysicalDevice device); 
    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device) const;
    static VkSurfaceFormatKHR pickSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    static VkPresentModeKHR pickPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
    static VkExtent2D chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    void createSwapchain(GLFWwindow* window);
    void createSwapchainImageViews();
    void initializeVMA();

    VkPipeline createGraphicsPipeline(VkShaderModule jointShaderModule) const;
    VkPipeline createComputePipeline(VkShaderModule computeShaderModule, const std::vector<VkPushConstantRange>& pcRanges, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) const;

    void createDescriptorPool();
    
    void createComputeSetLayout();
    void createComputeDescriptorSet();

    VkInstance mInstance;
    VmaAllocator mVmaAllocator;
    VkPhysicalDevice mPhysicalDevice;
    VkDevice mDevice;
  
    VkQueue mGraphicsQueue;
    VkQueue mComputeQueue;
    VkQueue mTransferQueue;
    VkQueue mPresentQueue;

    VkSurfaceKHR mWindowSurface;

    VkSurfaceFormatKHR mSwapchainFormat;
    VkPresentModeKHR mSwapchainPresentMode;
    VkExtent2D mSwapchainExtent;
    VkSwapchainKHR mSwapchain;
    std::vector<VkImage> mSwapchainImages;
    std::vector<VkImageView> mSwapchainImageViews;
    constexpr static VkFormat depthFormat{VK_FORMAT_D32_SFLOAT};

    VkImage mDepthImage;
    VkImageView mDepthImageView;
    VmaAllocation mDepthImageAllocation;

    VkDescriptorSetLayout mComputeSetLayout;
    VkDescriptorPool mComputeDescriptorPool;
    VkDescriptorSet mComputeDescriptorSet;
  };
}

#endif
