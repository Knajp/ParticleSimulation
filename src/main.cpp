#include "Application.hpp"
#include <iostream>

int main(int argc, char **argv) 
{
  for(int arg = 0; arg < argc; arg++)
    std::cout << argv[arg] << "\n";

  app::Application& APPLICATION = app::Application::getInstance();
  try{
    APPLICATION.RUN();
  }catch(const std::exception& e) {std::cout << e.what() << "\n";}

  return 0;
}
