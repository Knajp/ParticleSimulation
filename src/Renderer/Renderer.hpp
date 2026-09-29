#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <volk/volk.h>
#include <GLFW/glfw3.h>
#include <vma/vk_mem_alloc.h>

#include <optional>
#include <vector>
#include <limits>
#include <string>

#include "Shader.hpp"


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
      createDebugMessenger();
      createWindowSurface(window);
      pickPhysicalDevice();
      createLogicalDevice();
      initializeVMA();
      createSwapchain(window);      
      createGraphicsShaderModules();
      createDescriptorPool();
      createDescriptorSetLayout();
      createDescriptorSets();
      mGraphicsPipeline = createGraphicsPipeline(mVertexShaderModule, mFragmentShaderModule, mGraphicsPipelineLayout);
      createSynchronizationResources();
      createCommandBuffers();
    }
   
    void Step()
    {
      
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
      vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
      vkDestroyInstance(mInstance, nullptr);
    }

    VkDevice getDevice() const
    {
      return mDevice;
    }

    VmaAllocator getAllocator() const
    {
      return mVmaAllocator;
    }

    VkQueue getComputeQueue() const
    {
      return mComputeQueue;
    }
    
    uint32_t getComputeFamilyIndex() const
    {
      QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice);
      return indices.computeFamily.value();
    }
    
    void drawStorageBuffer(VkBuffer buffer, uint32_t vertexCount) const;

    VkCommandBuffer beginRecording();
    void beginRendering();
    void endAndSubmit();
  private:

    static constexpr int MAX_FRAMES_IN_FLIGHT = 3;
    int mCurrentFrameInFlight = 0;
    uint32_t mCurrentImageIndex = 0;

    void createVulkanInstance();
    void createDebugMessenger();
    void pickPhysicalDevice();
    void createLogicalDevice();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
    void createWindowSurface(GLFWwindow* window);
    static bool checkDeviceExtensionSupport(VkPhysicalDevice device); 
    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device) const;
    static VkSurfaceFormatKHR pickSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    static VkPresentModeKHR pickPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
    static VkExtent2D chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    void createSwapchain(GLFWwindow* window);
    void createSwapchainImageViews();
    void initializeVMA();
    void createCommandBuffers();
    void createGraphicsShaderModules();

    void createSynchronizationResources();
    VkPipeline createGraphicsPipeline(VkShaderModule vertexShaderModule, VkShaderModule fragmentShaderModule, VkPipelineLayout& pipelineLayout) const;

    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSets();

    VkInstance mInstance;
    VmaAllocator mVmaAllocator;
    VkPhysicalDevice mPhysicalDevice;
    VkDevice mDevice;
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
  
    VkQueue mGraphicsQueue;
    VkQueue mComputeQueue;
    VkQueue mTransferQueue;
    VkQueue mPresentQueue;

    VkShaderModule mVertexShaderModule;
    VkShaderModule mFragmentShaderModule;
    VkPipeline mGraphicsPipeline;
    VkPipelineLayout mGraphicsPipelineLayout;

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

    VkCommandPool mCommandPool;
    std::vector<VkCommandBuffer> mCommandBuffers;

    VkCommandPool mTransferPool;
    VkCommandBuffer mTransferBuffer;

    VkDescriptorPool mDescriptorPool;
    VkDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorSet mDescriptorSet;

    std::vector<VkSemaphore> mImageAvailableSemaphores;
    std::vector<VkSemaphore> mRenderFinishedSemaphores;
    std::vector<VkFence> mInFlightFences;
  };
}

#endif
