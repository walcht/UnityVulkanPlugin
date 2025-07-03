cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(TextureSubPlugin)

# Search packages for host system instead of packages for target system
# in case of cross compilation these macro should be defined by toolchain file
if(NOT COMMAND find_host_package)
  macro(find_host_package)
    find_package(${ARGN})
  endmacro()
endif()
if(NOT COMMAND find_host_program)
  macro(find_host_program)
    find_program(${ARGN})
  endmacro()
endif()


message(STATUS "building for target ${CMAKE_SYSTEM_NAME}")
message(STATUS "build type is ${CMAKE_BUILD_TYPE}")

# Unity's plugin API (PluginAPI folder) path
if(NOT IS_DIRECTORY ${UNITY_PLUGIN_API})
  message(FATAL_ERROR "target Unity's PluginAPI folder path has to provided by setting UNITY_PLUGIN_API")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(UNITY_LINUX 1)
endif()

set(SOURCES
    src/TextureSubPlugin.cpp
    src/TextureSubPluginAPI.cpp
)

if (SUPPORT_VULKAN)
  list(APPEND SOURCES src/TextureSubPluginAPI_Vulkan.cpp)
endif()

if (SUPPORT_OPENGL_CORE)
  list(APPEND SOURCES src/TextureSubPluginAPI_OpenGLCoreES.cpp)
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # glad path for OpenGL build on Windows
    if(NOT IS_DIRECTORY ${GLAD_PATH})
      message(FATAL_ERROR "glad (OpenGL loader) is not a valid directory - set it by assigning GLAD_PATH")
    endif()
    list(APPEND SOURCES ${GLAD_PATH}/src/glad.c)
  endif()
endif()

if (SUPPORT_D3D11)
  list(APPEND SOURCES src/TextureSubPluginAPI_D3D11.cpp)
endif()

# POSITION_INDEPENDENT_CODE property is True by default for SHARED targets
add_library(TextureSubPlugin SHARED ${SOURCES})

target_include_directories(TextureSubPlugin
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/
        ${UNITY_PLUGIN_API}
)

if(SUPPORT_OPENGL_CORE AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_include_directories(TextureSubPlugin
      PRIVATE
          ${GLAD_PATH}/include/
  )
endif()

if(SUPPORT_VULKAN)
  find_host_package(Vulkan)
  if(NOT Vulkan_FOUND)
    message(WARNING "could not find Vulkan SDK - unsetting SUPPORT_VULKAN")
    unset(SUPPORT_VULKAN)
  endif()
  target_link_libraries(TextureSubPlugin Vulkan::Vulkan)
endif()

if(SUPPORT_OPENGL_CORE)
  find_host_package(OpenGL)
  if(NOT OpenGL_FOUND)
    message(WARNING "could not find OpenGL - unsetting SUPPORT_OPENGL_CORE")
    unset(SUPPORT_OPENGL_CORE)
  endif()
  target_link_libraries(TextureSubPlugin OpenGL::GL)
endif()

if (ANDROID_ABI MATCHES x86_64)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lc++")
endif()

if(SUPPORT_OPENGL_ES)
  # TODO: add OpenGLES support
endif()

# pass preprocessor definitions
target_compile_definitions(TextureSubPlugin
    PRIVATE
        -DSUPPORT_VULKAN=${SUPPORT_VULKAN}
        -DSUPPORT_D3D11=${SUPPORT_D3D11}
        -DSUPPORT_OPENGL_CORE=${SUPPORT_OPENGL_CORE}
        -DSUPPORT_OPENGL_ES=${SUPPORT_OPENGL_ES}
        -DUNITY_LINUX=${UNITY_LINUX}
)

install(TARGETS TextureSubPlugin DESTINATION .)
install(FILES ${PROJECT_SOURCE_DIR}/TextureSubPlugin.cs DESTINATION .)
