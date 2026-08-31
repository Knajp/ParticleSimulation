#ifndef APPLICATION_HPP
#define APPLICATION_HPP
  
#include "Window.hpp"
#include "Renderer/Renderer.hpp"

namespace app
{
  class Application
  {
  public:
    static Application& getInstance()
    {
      static Application instance;
      return instance;
    }
    
    void RUN()
    {
      init();
      run();
      terminate();
    }
  private:
    void init();
    void run();
    void terminate();

  private:
    Window mWindow;
    ::rend::Renderer& mRenderer = rend::Renderer::getInstance();
  };
}

#endif
