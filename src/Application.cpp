#include "Application.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>

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
  }
  
  void Application::run()
  {
    while(!glfwWindowShouldClose(mWindow.getHandle()))
    {
      glfwPollEvents();
    }
  }

  void Application::terminate()
  {
    mRenderer.Terminate();
    mWindow.destroy();
    glfwTerminate();
  }
}
