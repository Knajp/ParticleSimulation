#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window
{
public:
  Window() = default;

  void init();
  void init(int width, int height);
  void destroy();

  ~Window();

  GLFWwindow* getHandle() const {return pWindow;}

private:
  GLFWwindow* pWindow = nullptr;
};
