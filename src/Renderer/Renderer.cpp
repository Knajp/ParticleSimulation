#include "Renderer.hpp"
#include <GLFW/glfw3.h>
#include <cassert>
#include <set>
#include <map>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

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

  VkDeviceQueueCreateInfo graphicsQueueCreateInfo{};
  graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  graphicsQueueCreateInfo.queueCount = 1;
  graphicsQueueCreateInfo.queueFamilyIndex =
      familyIndices.graphicsFamily.value();
  graphicsQueueCreateInfo.pQueuePriorities = &queuePriorities;

  VkDeviceQueueCreateInfo computeQueueCreateInfo{};
  computeQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  computeQueueCreateInfo.queueCount = 1;
  computeQueueCreateInfo.queueFamilyIndex = familyIndices.computeFamily.value();
  computeQueueCreateInfo.pQueuePriorities = &queuePriorities;

  VkDeviceQueueCreateInfo transferQueueCreateInfo{};
  transferQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  transferQueueCreateInfo.queueCount = 1;
  transferQueueCreateInfo.queueFamilyIndex =
      familyIndices.transferFamily.value();
  transferQueueCreateInfo.pQueuePriorities = &queuePriorities;

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

void Renderer::createWindowSurface(GLFWwindow *window) {
  if (glfwCreateWindowSurface(mInstance, window, nullptr, &mWindowSurface) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create GLFW window surface!");
}
} // namespace rend
