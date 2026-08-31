cmake_minimum_required(VERSION 3.20)

project(ParticleSimulation
  VERSION 1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(particle-sim 
  src/main.cpp
  src/Renderer/Renderer.cpp
  src/Application.cpp
  src/Window.cpp
)

target_include_directories(particle-sim PRIVATE
  vendor/include
)
target_link_libraries(particle-sim PRIVATE
  Vulkan::Vulkan
  glfw
)

target_compile_definitions(particle-sim PRIVATE
  GLFW_INCLUDE_VULKAN
)

