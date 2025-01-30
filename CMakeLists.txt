cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(TextureSubPlugin)

set(SOURCES
    src/TextureSubPlugin.cpp
    src/TextureSubPluginAPI.cpp
    src/TextureSubPluginAPI_OpenGLCoreES.cpp
    src/TextureSubPluginAPI_Vulkan.cpp
    src/TextureSubPluginAPI_D3D11.cpp
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(SOURCES ${SOURCES} extern/glad/glad.c)
endif()
add_library(TextureSubPlugin SHARED ${SOURCES})
set_property(TARGET TextureSubPlugin PROPERTY CXX_STANDARD 17)
target_include_directories(TextureSubPlugin PRIVATE include/ src/)
target_include_directories(TextureSubPlugin
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)
# compiler flags (CXXFLAGS)
target_compile_definitions(TextureSubPlugin
    PRIVATE
    #   -DSUPPORT_VULKAN=1
    #   -DSUPPORT_OPENGL_UNIFIED=1
    #   -DUNITY_LINUX=1
)
if (NOT MSVC)
    target_compile_options(TextureSubPlugin PRIVATE -O2 -fPIC)
endif()
# find OpenGL and link against it
find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)
target_link_libraries(TextureSubPlugin
    OpenGL::GL
    Vulkan::Vulkan
)
