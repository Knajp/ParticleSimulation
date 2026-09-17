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
find_package(glslang REQUIRED)

add_executable(particle-sim 
  src/main.cpp
  src/Renderer/Renderer.cpp
  src/Application.cpp
  src/Window.cpp
  src/Renderer/Pipeline.cpp
  src/Particle.cpp
  src/extern/vma.cpp
)

target_include_directories(particle-sim PRIVATE
  vendor/include
)
target_link_libraries(particle-sim PRIVATE
  Vulkan::Vulkan
  glfw
  glslang::glslang
  glslang::glslang-default-resource-limits
  glslang::SPIRV
  /home/lenovo/VulkanSDK/1.4.357.0/x86_64/lib/libvolk.a
)

target_compile_definitions(particle-sim PRIVATE
  GLFW_INCLUDE_VULKAN
  VK_NO_PROTOTYPES
)

