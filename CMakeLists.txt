cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(TextureSubPlugin)

# initally set all graphics API supports to 0
set(SUPPORT_VULKAN 0)
set(SUPPORT_D3D11 0)
set(SUPPORT_OPENGL_ES 0)
set(SUPPORT_OPENGL_CORE 0)

set(UNITY_LINUX 0)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")

  set(SUPPORT_D3D11 1 CACHE BOOL "include Direct3D11 support?")
  set(SUPPORT_VULKAN 0 CACHE BOOL "include Vulkan support?")

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

  set(SUPPORT_VULKAN 1 CACHE BOOL "include Vulkan support?")
  set(SUPPORT_OPENGL_CORE 0 CACHE BOOL "include OpenGL Core support?")
  set(SUPPORT_OPENGL_ES 0 CACHE BOOL "include OpenGL ES support?")
  set(UNITY_LINUX 1)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")

  set(SUPPORT_VULKAN 1 CACHE BOOL "include Vulkan support?")
  set(SUPPORT_OPENGL_ES 0 CACHE BOOL "include OpenGL ES support?")

endif()

set(SOURCES
    src/TextureSubPlugin.cpp
    src/TextureSubPluginAPI.cpp
)

if (SUPPORT_VULKAN)
  list(APPEND SOURCES src/TextureSubPluginAPI_Vulkan.cpp)
endif()

if (SUPPORT_OPENGL)
  list(APPEND SOURCES src/TextureSubPluginAPI_OpenGLCoreES.cpp)
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND SOURCES extern/glad/glad.c)
  endif()
endif()

if (SUPPORT_D3D11)
  list(APPEND SOURCES src/TextureSubPluginAPI_D3D11.cpp)
endif()

add_library(TextureSubPlugin SHARED ${SOURCES})

set_property(TARGET TextureSubPlugin PROPERTY CXX_STANDARD 17)

target_include_directories(TextureSubPlugin
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include/
        ${PROJECT_SOURCE_DIR}/src/
)

target_include_directories(TextureSubPlugin
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

# compiler definitions
target_compile_definitions(TextureSubPlugin
    PRIVATE
        -DSUPPORT_VULKAN=${SUPPORT_VULKAN}
        -DSUPPORT_OPENG_CORE=${SUPPORT_OPENGL_CORE}
        -DSUPPORT_OPENG_ES=${SUPPORT_OPENGL_ES}
        -DUNITY_LINUX=${UNITY_LINUX}
)

if(SUPPORT_VULKAN)
  find_package(Vulkan REQUIRED)
  target_link_libraries(TextureSubPlugin
    Vulkan::Vulkan
  )
endif()

if(SUPPORT_OPENGL)
  find_package(OpenGL REQUIRED)
  target_link_libraries(TextureSubPlugin
    OpenGL::GL
  )
endif()
