#include "Application.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>

void glfwErrorCallback(int num, const char* description)
{
  std::cerr << "GLFW error " << num << ": " << description << "\n";
}
void glfwFramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
  app::Application& appl = app::Application::getInstance();
  appl.WindowResize();
}
namespace app
{
  void Application::WindowResize()
  {
    mRenderer.signalFramebufferResize();
  }
  void Application::init()
  {
    glfwSetErrorCallback(glfwErrorCallback);
    if(!glfwInit())
      throw std::runtime_error("Failed to init GLFW!");
    mWindow.init();
    glfwSetFramebufferSizeCallback(mWindow.getHandle(), glfwFramebufferResizeCallback);
    mRenderer.Init(mWindow.getHandle());
    mParticleManager.init(mRenderer.getDevice(), mRenderer.getAllocator(), mRenderer.getComputeQueue(), mRenderer.getComputeFamilyIndex());  
  }
  
  void Application::run()
  {
    while(!glfwWindowShouldClose(mWindow.getHandle()))
    {
      glfwPollEvents();

      VkCommandBuffer cb = mRenderer.beginRecording();

      mParticleManager.step(mWindow.getHandle(), cb);

      mRenderer.beginRendering();

      mRenderer.drawStorageBuffer(mParticleManager.getBufferHandle(), part::ParticleManager::getParticleCount());

      mRenderer.endAndSubmit(mWindow.getHandle());
    }
  }

  void Application::terminate()
  {
    mRenderer.Terminate();
    mWindow.destroy();
    glfwTerminate();
  }
}
