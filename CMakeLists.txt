cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(TextureSubPlugin)
add_library(TextureSubPlugin SHARED
    src/TextureSubPlugin.cpp
    src/TextureSubPluginAPI.cpp
    src/TextureSubPluginAPI_OpenGLCoreES.cpp
    src/TextureSubPluginAPI_Vulkan.cpp
    src/TextureSubPluginAPI_D3D11.cpp
    extern/glad/glad.c
)
target_include_directories(TextureSubPlugin PRIVATE include/ src/)
target_include_directories(TextureSubPlugin
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)
# compiler flags (CXXFLAGS)
target_compile_definitions(TextureSubPlugin
    PRIVATE
        -DSUPPORT_OPENGL_UNIFIED=1
    #    -DSUPPORT_VULKAN=1
    #    -DUNITY_LINUX=1
)
if (NOT MSVC)
    target_compile_options(TextureSubPlugin PRIVATE -O2 -fPIC)
    target_link_options(TextureSubPlugin PRIVATE "LINKER:-rdynamic")
endif()
# find OpenGL and link against it
find_package(OpenGL REQUIRED)
target_link_libraries(TextureSubPlugin
    OpenGL::GL
)