#ifndef APPLICATION_HPP
#define APPLICATION_HPP
  
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
  };
}

#endif
