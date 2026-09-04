#include "Renderer.hpp"
#include <GLFW/glfw3.h>
#include <cassert>
#include <set>
#include <map>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <algorithm>

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

void Renderer::createVulkanInstance() {
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
      continue;
 
    bool swapchainAdequate = false;
    SwapchainSupportDetails swapchainDetails = querySwapchainSupport(device);
    swapchainAdequate = !swapchainDetails.presentModes.empty() && !swapchainDetails.surfaceFormats.empty();
    
    if(!swapchainAdequate)
      continue;

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

  std::set<const char*> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

  for(const auto& extension : extensions)
    requiredExtensions.erase(extension.extensionName);

  return requiredExtensions.empty();
}

void Renderer::createLogicalDevice() {
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
  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.enabledLayerCount = static_cast<uint32_t>(deviceLayers.size());
  createInfo.ppEnabledLayerNames = deviceLayers.data();
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
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

QueueFamilyIndices Renderer::findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilyProperties.data());

  int i = 0;
  for (const auto &qfp : queueFamilyProperties) {
    if (qfp.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      indices.graphicsFamily = i;
    if (qfp.queueFlags & VK_QUEUE_COMPUTE_BIT)
      indices.computeFamily = i;
    if (qfp.queueFlags & VK_QUEUE_TRANSFER_BIT)
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

SwapchainSupportDetails Renderer::querySwapchainSupport(VkPhysicalDevice device) const
{
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mWindowSurface, &details.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, mWindowSurface, &formatCount, nullptr);
  if(formatCount)
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, mWindowSurface, &formatCount, details.surfaceFormats.data());

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, mWindowSurface, &presentModeCount, nullptr);
  if(presentModeCount)
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, mWindowSurface, &presentModeCount, details.presentModes.data());

  return details;
    bool swapchainAdequate = false;
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
