#include "Application.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>

void glfwErrorCallback(int num, const char* description)
{
  std::cerr << "GLFW error " << num << ": " << description << "\n";
}
namespace app
{
  void Application::init()
  {
    glfwSetErrorCallback(glfwErrorCallback);
    if(!glfwInit())
      throw std::runtime_error("Failed to init GLFW!");
    mWindow.init();
    mRenderer.Init(mWindow.getHandle());
    mParticleManager.init(mRenderer.getDevice(), mRenderer.getAllocator(), mRenderer.getComputeQueue(), mRenderer.getComputeFamilyIndex());  
  }
  
  void Application::run()
  {
    while(!glfwWindowShouldClose(mWindow.getHandle()))
    {
      glfwPollEvents();

      mParticleManager.step(mWindow.getHandle());

      mRenderer.beginRecording();

      mRenderer.drawStorageBuffer(mParticleManager.getBufferHandle(), part::ParticleManager::getParticleCount());

      mRenderer.endAndSubmit();
    }
  }

  void Application::terminate()
  {
    mRenderer.Terminate();
    mWindow.destroy();
    glfwTerminate();
  }
}
