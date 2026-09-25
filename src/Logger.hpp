#ifndef LOGGER_H
#define LOGGER_H
#include <iostream>

class Logger
{
public:
  Logger(std::string name);
  Logger() = default;

  template <typename T>
  void info(T msg)
  {
    message(msg, "info");
  }

  template <typename T>
  void trace(T msg)
  {
    message(msg, "trace");
  }

  template <typename T>
  void debug(T msg)
  {
    message(msg, "debug");
  }
private:
std::string mName{};
  
  template <typename T>
  void message(T msg, std::string severity) // hey, its me, severity!
  {
    std::cout << mName << severity << ": " << msg;
  }
}

#endif
