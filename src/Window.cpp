#include "Window.hpp"
#include <GLFW/glfw3.h>
#include <cassert>

void Window::init()
{
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  assert(videoMode != nullptr);
  int width = videoMode->width;
  int height = videoMode->height;

  pWindow = glfwCreateWindow(width, height, "Particle Simulation", nullptr, nullptr);  
}

void Window::init(const int width, const int height)
{
  assert(width != 0 && height != 0);
  pWindow = glfwCreateWindow(width, height, "Particle Simulation", nullptr, nullptr);
}

void Window::destroy()
{
  assert(pWindow);
  glfwDestroyWindow(pWindow);
  pWindow = nullptr;
}

Window::~Window()
{
  if(pWindow)
    destroy();
}



